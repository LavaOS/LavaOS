#!/bin/sh
if [ -z "$BURN_ONTO" ]; then
    echo "$0: Missing path to install onto (Please specify BURN_ONTO=)"
    exit 1
fi

set -xe
clear

echo "[*]MinOS burn script"

sudo dd if=./bin/OS.iso of=$BURN_ONTO status=progress oflag=sync bs=1M
sync
sudo eject $BURN_ONTO

echo "[✓] Done."
