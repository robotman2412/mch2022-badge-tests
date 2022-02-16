
module top (
	output led_r_inv,
	output led_g_inv,
	output led_b_inv,
	
	inout  spi_miso,
	input  spi_mosi,
	input  spi_clk,
	input  spi_cs,
	
	input  clk
);
	
	localparam CMD_NEXT   = 8'b0000_1000;
	localparam CMD_TOGGLE = 8'b0010_0100;
	
	localparam clk_freq = 12_000_000;
	wire       next_r;
	wire       next_g;
	wire       next_b;
	reg        led_r;
	reg        led_g;
	reg        led_b;
	reg        is_the_on;
	reg[15:0]  pwm_reg;
	wire       pwm = pwm_reg[15:13] == 0;
	
	assign led_r_inv = ~(led_r & pwm & is_the_on);
	assign led_g_inv = ~(led_g & pwm & is_the_on);
	assign led_b_inv = ~(led_b & pwm & is_the_on);
	
	// SPI implmentation.
	reg[7:0] spi_recv_buf;
	reg[7:0] spi_send_buf;
	reg[2:0] spi_bit_counter;
	// Only drive the output if chip select is low (because it's active-low).
	assign  spi_miso = spi_cs ? 'bz : spi_send_buf[7];
	
	// assign led_g_inv = spi_cs;
	// assign led_b_inv = ~(spi_bit_counter == 7);
	
	initial begin
		// Wow so interesting.
		led_r = 0;
		led_g = 0;
		led_b = 0;
		is_the_on = 0;
		spi_bit_counter = 0;
		pwm_reg = 0;
		spi_cs_reg = 1;
	end
	
	always @(posedge clk) begin
		pwm_reg = pwm_reg + 1;
	end
	
	wire fancy_clk  = spi_cs | spi_clk;
	reg  spi_cs_reg = 1;
	always @(negedge fancy_clk) begin
		if (spi_cs_reg) begin
			// Prepare data.
			spi_bit_counter   = 0;
			spi_send_buf      = spi_recv_buf;
		end else begin
			// Shift transmit data.
			spi_bit_counter   = spi_bit_counter + 1;
			spi_send_buf[7:1] = spi_send_buf[6:0];
		end
	end
	
	always @(posedge fancy_clk) begin
		if (~spi_cs) begin
			// Perform transaction.
			spi_recv_buf[7:1] = spi_recv_buf[6:0];
			spi_recv_buf[0]   = spi_mosi;
			if (spi_bit_counter == 7) begin
				// We RECIEVED dATA!
				if (spi_recv_buf == CMD_TOGGLE) begin
					is_the_on = ~is_the_on;
				end else if (spi_recv_buf == CMD_NEXT) begin
					is_the_on = 1;
					if (led_r) begin
						led_r = 0;
						led_g = 1;
					end else if (led_g) begin
						led_g = 0;
						led_b = 1;
					end else begin
						led_b = 0;
						led_r = 1;
					end
				end
			end
		end
		spi_cs_reg = spi_cs;
	end
	
endmodule
