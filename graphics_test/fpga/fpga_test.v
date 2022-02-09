
module top (
    output led_r_inv,
    output led_g_inv,
    output led_b_inv,
    input  clk
);
    
    localparam clk_freq = 12_000_000;
    localparam limit = clk_freq / 512;
    reg[1:0]  phase;
    reg[8:0]  part;
    reg[31:0] counter;
    reg[7:0]  pwm_counter;
    
    reg  pwm;
    wire up_pwm   = part[8] ? 1 : pwm;
    wire down_pwm = part[8] ? ~pwm : 1;
    
    wire   led_r = (phase == 0) ? down_pwm :
                   (phase == 2) ? up_pwm   : 0;
    wire   led_g = (phase == 1) ? down_pwm :
                   (phase == 0) ? up_pwm   : 0;
    wire   led_b = (phase == 2) ? down_pwm :
                   (phase == 1) ? up_pwm   : 0;
    
    assign led_r_inv = ~led_r;
    assign led_g_inv = ~led_g;
    assign led_b_inv = ~led_b;
    
    initial begin
        phase   = 0;
        counter = limit;
    end
    
    always @(posedge clk) begin
        counter = counter - 1;
        // Phase your colors.
        if (counter == 0) begin
            counter = limit;
            // Next phase.
            if (part == 511) begin
                phase = (phase == 2) ? 0 : phase + 1;
            end
            part = part + 1;
        end
        
        // PWM.
        if (pwm_counter == part[7:0]) begin
            pwm = 0;
        end else if (pwm_counter == 0) begin
            pwm = 1;
        end
        pwm_counter = pwm_counter + 1;
    end
    
endmodule
