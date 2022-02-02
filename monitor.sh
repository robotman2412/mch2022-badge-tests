#!/bin/bash

cd "$FIRMWARE_PATH"
idf.py -p "$PORT" flash && screen "$PORT" 115200
