#!/bin/bash

cd "$FIRMWARE_PATH"
idf.py flash && screen /dev/ttyUSB* 115200
