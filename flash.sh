#!/bin/bash

cd "$FIRMWARE_PATH"
idf.py -p "$PORT" flash
