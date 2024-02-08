`timescale 1ns / 1ps

`define assert(signal, value) \
    if (signal !== value) begin \
        $display("ASSERTION FAILED in %m: signal != value"); \
        $finish; \
    end

module FIFO_TB;

reg clk;
reg rst;
reg write_en;
reg [31:0] write_data;
reg read;

wire [31:0] read_data;
wire fifo_full;
wire fifo_empty;

MY_FIFO #(4) fifo (
    clk,
    rst,
    write_en,
    write_data,
    read,
    read_data,
    fifo_full,
    fifo_empty
);

initial begin
    clk = 0;
    forever #0.5 clk = ~clk;
end

initial begin

    rst = 1;
    write_en = 0;
    write_data = 32'b0;
    read = 0;
    
    #5
    rst = 0;
    #1;
    `assert(fifo_empty, 1'b1)
    
    write_en = 1;
    write_data = 32'h1;
    #1;
    `assert(fifo_empty, 1'b0)
    `assert(fifo_full, 1'b0)
    
    write_data = 32'h2;
    #1;
    write_data = 32'h3;
    #1;
    write_data = 32'h4;
    #1;
    write_en = 0;
    `assert(fifo_empty, 1'b0)
    `assert(fifo_full, 1'b1)
    
    read = 1;
    `assert(read_data, 32'h1)
    #1;
    `assert(fifo_empty, 1'b0)
    `assert(fifo_full, 1'b0)
    `assert(read_data, 32'h2)
    #1;
    `assert(read_data, 32'h3)
    #1;
    `assert(read_data, 32'h4)
    #1;
    read = 0;
    `assert(fifo_empty, 1'b1)
    `assert(fifo_full, 1'b0)
    #10;
    
    write_en = 1;
    write_data = 32'h1;
    `assert(fifo_empty, 1'b1)
    #1;
    read = 1;
    write_data = 32'h2;
    `assert(fifo_empty, 1'b0)
    `assert(read_data, 32'h1)
    #1;
    write_data = 32'h3;
    `assert(fifo_empty, 1'b0)
    `assert(read_data, 32'h2)
    #1;
    write_data = 32'h4;
    `assert(read_data, 32'h3)
    #1;
    write_data = 32'h5;
    `assert(read_data, 32'h4)
    #1;
    write_en = 0;
    `assert(read_data, 32'h5)
    #1;
    `assert(fifo_empty, 1'b1)
    read = 0;
    
end

endmodule
