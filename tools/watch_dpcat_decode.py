#!/usr/bin/env python3

"""Watch DpCat for new data-product files and decode them to JSON.

Run this from the project venv on the GDS/server machine.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import time
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
# fprime-gds files everything it downlinks under a fixed "fprime-downlink"
# subdirectory of --file-storage-directory (DpCat/, per docker-compose.yml)
# - it never writes directly into DpCat/ itself.
DEFAULT_WATCH_DIR = REPO_ROOT / "DpCat" / "fprime-downlink"
DEFAULT_DICTIONARY = REPO_ROOT / "build-artifacts/aarch64-linux/BeeDeployment/dict/BeeDeploymentTopologyDictionary.json"

# Resolve fprime-dp next to the running interpreter first so this still works when
# launched outside an activated venv shell (e.g. under systemd, where PATH is minimal).
FPRIME_DP = Path(sys.executable).parent / "fprime-dp"
if not FPRIME_DP.is_file():
    FPRIME_DP = shutil.which("fprime-dp") or "fprime-dp"


def resolve_repo_path(path: Path) -> Path:
    if path.is_absolute():
        return path

    return (REPO_ROOT / path).resolve()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Watch a DpCat directory and decode any new .fdp files to JSON."
    )
    parser.add_argument(
        "--watch-dir",
        type=Path,
        default=DEFAULT_WATCH_DIR,
        help="Directory to watch for .fdp files (default: DpCat)",
    )
    parser.add_argument(
        "--dictionary",
        type=Path,
        default=DEFAULT_DICTIONARY,
        help="BeeDeployment dictionary to use for decoding",
    )
    parser.add_argument(
        "--poll-interval",
        type=float,
        default=2.0,
        help="Seconds to wait between scans of DpCat",
    )
    return parser.parse_args()


def decode_file(bin_file: Path, dictionary: Path) -> None:
    output_file = bin_file.with_suffix(".json")
    command = [
        str(FPRIME_DP),
        "decode",
        "--bin-file",
        str(bin_file),
        "--dictionary",
        str(dictionary),
        "--output",
        str(output_file),
    ]
    print(f"Decoding {bin_file.name} -> {output_file.name}")
    subprocess.run(command, check=True)


def main() -> int:
    args = parse_args()
    watch_dir = resolve_repo_path(args.watch_dir)
    dictionary = resolve_repo_path(args.dictionary)

    if not Path(FPRIME_DP).is_file() and shutil.which(str(FPRIME_DP)) is None:
        print(
            f"fprime-dp was not found (looked for {FPRIME_DP}). "
            "Activate the project venv before running this script.",
            file=sys.stderr,
        )
        return 1

    if not dictionary.is_file():
        print(f"Dictionary not found: {dictionary}", file=sys.stderr)
        return 1

    print(f"Watching {watch_dir} for new .fdp files")
    print(f"Using dictionary {dictionary}")

    last_signatures: dict[Path, tuple[int, int]] = {}
    stable_counts: dict[Path, int] = {}
    # Which (size, mtime) signature each .fdp was last successfully decoded
    # at. This — not "does the .json output still exist" — is the source of
    # truth for "have I already decoded this." A downstream consumer of the
    # decoded JSON (e.g. a watcher that ingests it into a database) may move
    # or delete that output once it's done with it; if this script re-checked
    # the filesystem for its own output every poll, it would see that as
    # "not decoded yet" and redecode forever, in lockstep with however often
    # the consumer clears it out. Remembering what we've already decoded
    # in-process avoids that race entirely, at the cost of redecoding once
    # more per file if this script itself restarts — a bounded one-time
    # catch-up, not a loop.
    decoded_signatures: dict[Path, tuple[int, int]] = {}

    while True:
        if not watch_dir.is_dir():
            time.sleep(args.poll_interval)
            continue

        # iterdir(), not glob("*.fdp") - fprime-gds's FileDownlinker.sanitize()
        # turns any "/" in the flight-side source path into "_", and this
        # deployment's dp directory is configured as "./DpCat", so downlinked
        # filenames start with "._" (e.g. "._DpCat_Dp_...fdp"). glob's "*"
        # doesn't match a leading dot, so it would silently skip every file.
        current_files = sorted(
            (path for path in watch_dir.iterdir() if path.is_file() and path.suffix == ".fdp"),
            key=lambda path: (path.stat().st_mtime_ns, path.name),
        )

        current_paths = set(current_files)
        for path in list(last_signatures):
            if path not in current_paths:
                last_signatures.pop(path, None)
                stable_counts.pop(path, None)
                decoded_signatures.pop(path, None)

        for path in current_files:
            try:
                stat_result = path.stat()
            except FileNotFoundError:
                continue
            signature = (stat_result.st_size, stat_result.st_mtime_ns)
            previous_signature = last_signatures.get(path)

            if previous_signature != signature:
                last_signatures[path] = signature
                stable_counts[path] = 0
                continue

            stable_counts[path] = stable_counts.get(path, 0) + 1
            already_decoded = decoded_signatures.get(path) == signature

            if stable_counts[path] >= 1 and not already_decoded:
                try:
                    decode_file(path, dictionary)
                    decoded_signatures[path] = signature
                except subprocess.CalledProcessError as error:
                    print(f"Failed to decode {path.name}: {error}", file=sys.stderr)

        time.sleep(args.poll_interval)


if __name__ == "__main__":
    raise SystemExit(main())