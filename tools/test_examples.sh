#!/bin/bash

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <IP:PORT> <TOKEN>"
    exit 1
fi

IP="$1"
TOKEN="$2"

python send_script.py "$IP" "$TOKEN" examples --recursive