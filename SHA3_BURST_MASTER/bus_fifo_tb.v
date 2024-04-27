`timescale 1ns / 1ps

`define assert(signal, value) \
    if (signal !== value) begin \
        $display("ASSERTION FAILED in %m: signal != value"); \
        $finish; \
    end

`define combine(a, b) {a, b}

module bus_fifo_tb;

reg clk;
reg rst;
reg write_en;
reg [127:0] write_data;
reg read_en;

wire [127:0] read_data;
wire fifo_full;
wire fifo_empty;
wire fifo_half_full;

Bus_FIFO #(8) bus_fifo (
    .clk(clk),
    .rst(rst),
    .write_data(write_data),
    .write_en(write_en),
    .read_en(read_en),
    .read_data(read_data),
    .fifo_full(fifo_full),
    .fifo_half_full(fifo_half_full),
    .fifo_empty(fifo_empty)
);

initial begin
    clk = 1;
    forever #0.5 clk = ~clk;
end

initial begin

    rst = 1;
    write_en = 0;
    write_data = 128'b0;
    read_en = 0;
    
    #5
    rst = 0;
    #1;
    `assert(fifo_empty, 1'b1)
    
    // Test 1: Fill up and empty the FIFO
    write_en = 1;
    write_data = 128'd0; #1; 
    write_data = 128'd1; #1; 
    write_data = 128'd2; #1;
    write_data = 128'd3; #1;
    `assert(fifo_half_full, 1'b1)

    write_data = 128'd4; #1;
    write_data = 128'd5; #1;
    write_data = 128'd6; #1;
    write_data = 128'd7; #1;
    `assert(fifo_empty, 1'b0)
    `assert(fifo_full, 1'b1)

    write_en = 0; read_en = 1; #1;
    `assert(read_data, 128'd0) #1;
    `assert(fifo_full, 1'b0)
    `assert(read_data, 128'd1) #1;
    `assert(read_data, 128'd2) #1;
    `assert(read_data, 128'd3) #1;
    `assert(fifo_half_full, 1'b0)
    `assert(read_data, 128'd4) #1;
    `assert(read_data, 128'd5) #1;
    `assert(read_data, 128'd6) #1;
    `assert(read_data, 128'd7)
    `assert(fifo_empty, 1'b1)
    read_en = 0;
    #5;

    $finish;
    
end

initial begin
  $dumpfile ("bus_fifo_tb.vcd");
  $dumpvars (0, bus_fifo);
  #1;
end

endmodule
