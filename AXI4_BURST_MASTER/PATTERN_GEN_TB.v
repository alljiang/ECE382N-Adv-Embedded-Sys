`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01/26/2024 06:15:18 PM
// Design Name: 
// Module Name: PATTERN_GEN_TB
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////
`define assert(signal, value) \
    if (signal !== value) begin \
        $display("ASSERTION FAILED in %m: signal != value"); \
        $finish; \
    end

module PATTERN_GEN_TB;

    reg clk;
    reg rst;
    reg stall;
    reg [1:0] pattern_sel;
    reg [31:0] seed;
    
    wire [31:0] pattern_out;    

    PATTERN_GEN pattern_gen(
        clk,
        rst,
        stall,
        pattern_sel,
        seed,
        pattern_out
    );
    
    initial begin
        clk = 0;
        
        forever #0.5 clk = ~clk;
        
    end
    
    initial begin
        clk = 0;
        rst = 1;
        stall = 0;
        pattern_sel = `PATTERN_SLIDING_ZERO;
        seed = 32'hDEADFEED;
        
        #10
        rst = 0;
        
        pattern_sel = `PATTERN_SLIDING_ZERO;
        `assert(pattern_out, 32'hFFFFFFFE)
        #1
        `assert(pattern_out, 32'hFFFFFFFD)
        #30
        `assert(pattern_out, 32'h7FFFFFFF)
        stall = 1;
        #1;
        `assert(pattern_out, 32'h7FFFFFFF)
        #1;
        `assert(pattern_out, 32'h7FFFFFFF)
        stall = 0;
        #1
        `assert(pattern_out, 32'hFFFFFFFE)
        #63
        
        pattern_sel = `PATTERN_SLIDING_ONE;
        #1
        `assert(pattern_out, 32'h1)
        #1
        `assert(pattern_out, 32'h2)
        #30
        `assert(pattern_out, 32'h80000000)
        #1
        `assert(pattern_out, 32'h1)
        
        pattern_sel = `PATTERN_LFSR;
    end
    
endmodule
