#!/bin/bash

cd "$FIRMWARE_PATH"
make_fpga.sh && idf.py -p "$PORT" flash
