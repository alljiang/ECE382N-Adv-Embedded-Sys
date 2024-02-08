`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01/26/2024 05:08:59 PM
// Design Name: 
// Module Name: PATTERN_GEN
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

`define PATTERN_SLIDING_ZERO    (2'b00)
`define PATTERN_SLIDING_ONE     (2'b01)
`define PATTERN_LFSR            (2'b10)

module PATTERN_GEN(
    input clk,
    input rst,
    input stall,
    input [1:0] pattern_sel,
    input [31:0] seed,
    output reg [31:0] pattern_out
    );
    
    reg [31:0] lfsr;
    reg [31:0] sliding_zero;
    reg [31:0] sliding_one;
    
    always @(posedge clk) begin
        if (rst) begin
            lfsr <= seed;
            sliding_one <= 32'h00000001;
            sliding_zero <= 32'hFFFFFFFE;
        end
        else if (!stall) begin
            lfsr[31:1] <= lfsr[30:0];
            lfsr[0] <= lfsr[31] ^ lfsr[21] ^ lfsr[1] ^ lfsr[0];
            
            sliding_zero[0] <= sliding_zero[31]; 
            sliding_zero[31:1] <= sliding_zero[30:0];
            
            sliding_one[0] <= sliding_one[31]; 
            sliding_one[31:1] <= sliding_one[30:0]; 
        end
    end
    
    always @(*) begin
        case (pattern_sel)
            `PATTERN_SLIDING_ZERO: pattern_out = sliding_zero;
            `PATTERN_SLIDING_ONE: pattern_out = sliding_one;
            `PATTERN_LFSR: pattern_out = lfsr;
            default: pattern_out = 32'hDEADBEEF;
        endcase
    end
    
endmodule
