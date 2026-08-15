#!/usr/bin/env bash
# Copies the aarch64-linux BeeDeployment binary and its boot sequence
# (boot_dp_downlink.bin - see README.md section 6) to a Raspberry Pi.
#
# Run this from the dev machine, after building with:
#   fprime-util build aarch64-linux
#
# Usage: ./deploy_to_pi.sh <user>@<pi-host> [remote-dir]
#   remote-dir defaults to the user's home directory on the Pi.

set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <user>@<pi-host> [remote-dir]" >&2
    exit 1
fi

pi_target="$1"
remote_dir="${2:-}"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
binary="$repo_root/build-artifacts/aarch64-linux/BeeDeployment/bin/BeeDeployment"
boot_seq="$repo_root/BeeDeployment/boot_dp_downlink.bin"

if [[ ! -x "$binary" ]]; then
    echo "Missing binary: $binary" >&2
    echo "Build it first with: fprime-util build aarch64-linux" >&2
    exit 1
fi

if [[ ! -f "$boot_seq" ]]; then
    echo "Missing boot sequence: $boot_seq" >&2
    exit 1
fi

if [[ -n "$remote_dir" ]]; then
    ssh "$pi_target" "mkdir -p '$remote_dir'"
    remote_path="$remote_dir"
else
    remote_dir_expanded="$(ssh "$pi_target" 'echo $HOME')"
    remote_path="$remote_dir_expanded"
fi

# Upload to temp names, then rename into place, rather than scp-ing directly
# over BeeDeployment - if a previous copy is currently running (e.g. under
# the systemd service from tools/install_beedeployment_service.sh), writing
# straight over its executable fails with ETXTBSY. rename() doesn't have
# that problem: the running process keeps using its already-open inode
# until it's next restarted, and the new binary is what a `systemctl
# restart` (or the next boot) will pick up.
scp "$binary" "$pi_target:$remote_path/BeeDeployment.new"
scp "$boot_seq" "$pi_target:$remote_path/boot_dp_downlink.bin.new"
ssh "$pi_target" "chmod +x '$remote_path/BeeDeployment.new' && mv '$remote_path/BeeDeployment.new' '$remote_path/BeeDeployment' && mv '$remote_path/boot_dp_downlink.bin.new' '$remote_path/boot_dp_downlink.bin'"

echo "Deployed BeeDeployment and boot_dp_downlink.bin to $pi_target"
echo "If beedeployment.service is running, restart it to pick up the new binary:"
echo "  ssh $pi_target 'sudo systemctl restart beedeployment.service'"
echo "Both must land in the same directory - it's what BeeDeployment's"
echo "working directory needs to be when it (or the systemd service from"
echo "tools/install_beedeployment_service.sh) runs."
