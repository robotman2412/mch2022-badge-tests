#!/bin/bash

cd "$FIRMWARE_PATH/fpga"
if make; then
    echo '[1;102;30m''FPGA bitstream built''[0m'
else
    echo '[1;101m''FPGA bitstream error''[0m'
    exit 1
fi

