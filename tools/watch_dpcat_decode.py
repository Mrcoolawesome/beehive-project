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
DEFAULT_WATCH_DIR = REPO_ROOT / "DpCat"
DEFAULT_DICTIONARY = REPO_ROOT / "build-artifacts/Linux/BeeDeployment/dict/BeeDeploymentTopologyDictionary.json"


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
        "fprime-dp",
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

    if shutil.which("fprime-dp") is None:
        print(
            "fprime-dp was not found on PATH. Activate the project venv before running this script.",
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

    while True:
        if not watch_dir.is_dir():
            time.sleep(args.poll_interval)
            continue

        current_files = sorted(
            (path for path in watch_dir.glob("*.fdp") if path.is_file()),
            key=lambda path: (path.stat().st_mtime_ns, path.name),
        )

        current_paths = set(current_files)
        for path in list(last_signatures):
            if path not in current_paths:
                last_signatures.pop(path, None)
                stable_counts.pop(path, None)

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
            output_file = path.with_suffix(".json")
            try:
                output_exists = output_file.exists()
                output_is_current = output_exists and output_file.stat().st_mtime_ns >= stat_result.st_mtime_ns
            except FileNotFoundError:
                output_is_current = False

            if stable_counts[path] >= 1 and not output_is_current:
                try:
                    decode_file(path, dictionary)
                except subprocess.CalledProcessError as error:
                    print(f"Failed to decode {path.name}: {error}", file=sys.stderr)

        time.sleep(args.poll_interval)


if __name__ == "__main__":
    raise SystemExit(main())