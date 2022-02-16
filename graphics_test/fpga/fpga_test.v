`default_nettype none
`include "fpga_test_lcd.v"

module top (
	output      led_r_inv,
	output      led_g_inv,
	output      led_b_inv,
	
	inout       spi_miso,
	input       spi_mosi,
	input       spi_clk,
	input       spi_cs,
	
	input       clk_12mhz,
	output      esp_irq,
	
	inout [3:0] ram_data,
	output      ram_cs,
	output      ram_clk,
	
	output[7:0] lcd_data,
	output      lcd_regsel,
	output      lcd_write,
	input       lcd_fmark,
);
	
	wire clk_48mhz;
	SB_HFOSC #(.CLKHF_DIV("0b01")) hfosc0 (
		.CLKHFEN(1'b1),
		.CLKHFPU(1'b1),
		.CLKHF(clk_48mhz)
	);
	
	/* ==== El screen, of demo ==== */
	reg[8:0]   init_data;
	reg        init_rdy;
	reg        init_done;
	wire       init_ack;
	
	reg el_test;
	assign led_r_inv = spi_cs;
	assign led_g_inv = ~init_done;
	assign led_b_inv = ~el_test;
	
	initial begin
		init_rdy  = 0;
		init_done = 0;
		el_test   = 0;
	end
	
	lcd el_screen(
		.i_reset(0),
		.i_clk(clk_48mhz),
		.i_lcd_fmark(lcd_fmark),
		.o_lcd_wr(lcd_write),
		.o_lcd_rs(lcd_regsel),
		.o_lcd_data(lcd_data),
		
		.i_init_rom(init_data),
		.i_init_rdy(init_rdy),
		.i_init_done(init_done),
		.o_init_ack(init_ack)
	);
	
	/* ==== SPI recieve for init ROM ==== */
	
	// SPI parameters.
	localparam spi_bits     = 16;
	localparam spi_ctr_size = 4;
	
	// SPI registers.
	reg[spi_bits-1:0]       spi_recv_buf;
	reg[spi_bits-1:0]       spi_send_buf;
	reg[spi_ctr_size-1:0]   spi_bit_counter;
	
	// Only drive the output if chip select is low (because it's active-low).
	assign     spi_miso     = spi_cs ? 'bz : spi_send_buf[spi_bits-1];
	
	// This fancy clock makes sending possible.
	wire       fancy_clk    = spi_cs | spi_clk;
	reg        spi_cs_reg   = 1;
	
	always @(negedge fancy_clk) begin
		if (spi_cs_reg) begin
			// Prepare data.
			spi_bit_counter   = 0;
			spi_send_buf      = spi_recv_buf;
		end else begin
			// Shift transmit data.
			spi_bit_counter   = spi_bit_counter + 1;
			spi_send_buf[spi_bits-1:1] = spi_send_buf[spi_bits-2:0];
		end
	end
	
	always @(posedge fancy_clk) begin
		if (init_ack) begin
			init_rdy = 0;
		end
		
		if (~spi_cs) begin
			// Perform transaction.
			spi_recv_buf[spi_bits-1:1] = spi_recv_buf[spi_bits-2:0];
			spi_recv_buf[0]            = spi_mosi;
			if (spi_bit_counter == spi_bits-1) begin
				// We RECIEVED dATA!
				if (spi_recv_buf == 16'hcafe) begin
					el_test = 1;
				end
				if (spi_recv_buf[15]) begin
					// End of init ROM.
					init_done = 1;
				end else begin
					// Init ROM data.
					init_data = spi_recv_buf[8:0];
					init_rdy  = 1;
				end
			end
		end
		spi_cs_reg = spi_cs;
	end
	
endmodule
