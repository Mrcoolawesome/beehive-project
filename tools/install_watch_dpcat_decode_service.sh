#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
unit_dir="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user"
unit_file="$unit_dir/beehive-dpcat-decode.service"
watch_script="$repo_root/tools/watch_dpcat_decode.py"
venv_python="$repo_root/fprime-venv/bin/python"
dictionary="$repo_root/build-artifacts/arm-hf-linux/BeeDeployment/dict/BeeDeploymentTopologyDictionary.json"

if [[ ! -x "$venv_python" ]]; then
    echo "Missing Python venv executable: $venv_python" >&2
    exit 1
fi

if [[ ! -f "$watch_script" ]]; then
    echo "Missing watcher script: $watch_script" >&2
    exit 1
fi

if [[ ! -f "$dictionary" ]]; then
    echo "Missing dictionary: $dictionary" >&2
    exit 1
fi

mkdir -p "$unit_dir"

cat >"$unit_file" <<EOF
[Unit]
Description=Beehive DpCat auto-decoder
After=default.target

[Service]
Type=simple
WorkingDirectory=$repo_root
ExecStart=$venv_python $watch_script --watch-dir $repo_root/DpCat --dictionary $dictionary
Restart=always
RestartSec=5
Environment=PYTHONUNBUFFERED=1

[Install]
WantedBy=default.target
EOF

systemctl --user daemon-reload
systemctl --user enable --now beehive-dpcat-decode.service

echo "Installed and started: $unit_file"
echo "To keep it running after logout, optionally enable lingering with: loginctl enable-linger \"$USER\""