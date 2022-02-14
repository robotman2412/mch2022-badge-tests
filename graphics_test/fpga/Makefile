
.PHONY: all
all: fpga_test.h

# Synthesis.
fpga_test.json: fpga_test.v
	yosys -p 'synth_ice40 -top top -json fpga_test.json' $<

# Do place and route.
fpga_test.asc: fpga_test.json fpga_test_pins.pcf
	nextpnr-ice40 --up5k --package sg48 --json fpga_test.json --pcf fpga_test_pins.pcf --asc $@

# Convert the .asc file to a .bin file.
fpga_test.bin: fpga_test.asc
	icepack $< $@

# Convert the .bin file to a C .h file.
fpga_test.h: fpga_test.bin
	echo 'unsigned const char fpga_test_bin[] = {' > fpga_test.h
	xxd -i < fpga_test.bin >> fpga_test.h
	echo '};' >> fpga_test.h
	echo 'unsigned int fpga_test_bin_len = sizeof(fpga_test_bin) / sizeof(unsigned char);' >> fpga_test.h

# El cleanening
.PHONY: clean
clean:
	$(RM) fpga_test.json fpga_test.asc fpga_test.h fpga_test.bin
