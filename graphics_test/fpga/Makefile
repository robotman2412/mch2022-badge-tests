
.PHONY: all
all: fpga_test.h

# Use Yosys to create an object file
fpga_test.blif: fpga_test.v
	yosys -q -p "read_verilog -noautowire $^ ; check ; clean ; synth_ice40 -blif $@"

# Use the object file + the constraint file to create bit field file
fpga_test.txt: fpga_test.blif fpga_test_pins.pcf
	arachne-pnr -d 5k -P sg48 -p $(word 2,$^) $< -o $@
# 	nextpnr-ice40 --package sg48 --up5k --pcf $(word 2,$^) $< --asc $@
# -o $@
# -d 5k
# -P sg48

# Convert the .bin file to a C .h file.
fpga_test.h: fpga_test.bin
	echo 'unsigned const char fpga_test_bin[] = {' > fpga_test.h
	xxd -i < fpga_test.bin >> fpga_test.h
	echo '};' >> fpga_test.h
	echo 'unsigned int fpga_test_bin_len = sizeof(fpga_test_bin) / sizeof(unsigned char);' >> fpga_test.h

# Pack the bit field file in a format that can be programmed into the FPGA
fpga_test.bin: fpga_test.txt
	icepack $< $@

.PHONY: flash
flash: fpga_test.bin
	iceprog fpga_test.bin

.PHONY: clean
clean:
	$(RM) -f fpga_test.blif fpga_test.txt fpga_test.ex fpga_test.bin fpga_test.h

.PHONY: show
show: fpga_test.blif
	yosys -p "synth_ice40; show" fpga_test.blif