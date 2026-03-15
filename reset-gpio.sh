#!/bin/bash

PIN=21

# Check if the first argument is "--delayed"
if [[ "$1" == "--delayed" ]]; then
    echo "Boot mode detected. Waiting 30s before reset..."
    sleep 30
else
    echo "Manual mode: Executing reset immediately."
fi

# Reset Sequence
echo "Pulling GPIO $PIN LOW..."
pinctrl set $PIN op dl

sleep 2

echo "Pulling GPIO $PIN HIGH..."
pinctrl set $PIN op dh

echo "Reset sequence complete."
