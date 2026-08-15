#!/usr/bin/env bash
# Installs a systemd service that starts BeeDeployment automatically on
# every boot of the Raspberry Pi it's run on.
#
# Run this ON THE PI, from whichever directory BeeDeployment (and
# boot_dp_downlink.bin) have already been copied to - see README.md
# section 8. It is not meant to run on the dev machine or the GDS/server.
#
# Usage: ./install_beedeployment_service.sh [gds-host] [gds-port] [user]
#   gds-host defaults to 100.122.230.118 (the GDS/server's Tailscale address)
#   gds-port defaults to 50000
#   user     defaults to whoever invokes this script

set -euo pipefail

gds_host="${1:-100.122.230.118}"
gds_port="${2:-50000}"
run_user="${3:-$USER}"

deploy_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
binary="$deploy_dir/BeeDeployment"
unit_file="/etc/systemd/system/beedeployment.service"

if [[ ! -x "$binary" ]]; then
    echo "Missing or non-executable binary: $binary" >&2
    echo "Copy the aarch64-linux BeeDeployment build here first (see README.md)." >&2
    exit 1
fi

if [[ ! -f "$deploy_dir/boot_dp_downlink.bin" ]]; then
    echo "Warning: boot_dp_downlink.bin not found next to $binary" >&2
    echo "Data-product downlink won't start automatically without it (see README.md)." >&2
fi

sudo tee "$unit_file" >/dev/null <<EOF
[Unit]
Description=BeeDeployment F' flight software
After=network-online.target bluetooth.service tailscaled.service
Wants=network-online.target bluetooth.service

[Service]
Type=simple
User=$run_user
WorkingDirectory=$deploy_dir
ExecStart=$binary -a $gds_host -p $gds_port
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable --now beedeployment.service

echo "Installed and started: $unit_file"
echo "Watch it with: journalctl -u beedeployment -f"
