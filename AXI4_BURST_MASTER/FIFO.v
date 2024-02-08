`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01/27/2024 02:26:32 AM
// Design Name: 
// Module Name: FIFO
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


module MY_FIFO #(parameter depth=8)
(
    input clk,
    input rst,
    input write_en,
    input [31:0] write_data,
    input read,
    output [31:0] read_data,
    output fifo_full,
    output fifo_empty
);

    reg [31:0] memory[7:0];

    assign memory[0] = 8'hfffffff0;
    assign memory[1] = 8'hfffffff1;
    assign memory[2] = 8'hfffffff2;
    assign memory[3] = 8'hfffffff3;
    assign memory[4] = 8'hfffffff4;
    assign memory[5] = 8'hfffffff5;
    assign memory[6] = 8'hfffffff6;
    assign memory[7] = 8'hfffffff7;
    
    // reg [31:0] memory[depth:0];
    reg [$clog2(depth)-1:0] write_ptr;
    reg [$clog2(depth)-1:0] read_ptr;
    reg [$clog2(depth):0] count;
    
    assign fifo_full = count == depth;
    assign fifo_empty = count == 0;
    assign read_data[31:0] = memory[read_ptr];
    
    always @(posedge clk) begin
        if (rst) begin
            count <= 0;
        end
        else begin
            case ({write_en, read})
            // assume FIFO will never be read while empty or written while full
                2'b00, 2'b11: count <= count;
                2'b01: count <= count - 1'b1;
                2'b10: count <= count + 1'b1;
            endcase
        end
    end
    
    always @(posedge clk) begin    
        if (rst) begin
            read_ptr <= 0;
        end
        else begin
            if (read & !fifo_empty) begin
                read_ptr <= read_ptr + 1'b1;
            end
        end
    
    end
    
    always @(posedge clk) begin
    
        if (rst) begin
            write_ptr <= 0;
        end
        else begin
            if (write_en & !fifo_full) begin
                write_ptr <= write_ptr + 1;
                // memory[write_ptr] <= write_data;
            end
        end
    
    end
    
endmodule
