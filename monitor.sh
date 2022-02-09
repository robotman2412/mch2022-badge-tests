#!/bin/bash

cd "$FIRMWARE_PATH"
make_fpga.sh && idf.py -p "$PORT" flash && screen "$PORT" 115200
