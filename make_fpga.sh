#!/bin/bash

cd "$FIRMWARE_PATH/fpga"
make && echo "FPGA bitstream built" || echo "FPGA bitstream error"
