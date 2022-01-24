#!/bin/bash

cd "$FIRMWARE_PATH"
addr2line -e build/main.elf $*
