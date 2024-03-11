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

wire [63:0] read_data;
wire fifo_full;
wire fifo_empty;
wire fifo_half_full;

Bus_FIFO #(16) bus_fifo (
    .clk(clk),
    .rst(rst),
    .write_data(write_data),
    .write_en(write_en),
    .read(read_en),
    .read_data(read_data),
    .fifo_full(fifo_full),
    .fifo_half_full(fifo_half_full),
    .fifo_empty(fifo_empty)
);

initial begin
    clk = 0;
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
    write_data = `combine(128'h1, 128'h0); #1; 
    write_data = `combine(128'h3, 128'h2); #1;
    write_data = `combine(128'h5, 128'h4); #1;
    write_data = `combine(128'h7, 128'h6); #1;
    `assert(fifo_half_full, 1'b1)

    write_data = `combine(128'h9, 128'h8); #1;
    write_data = `combine(128'h11, 128'h10); #1;
    write_data = `combine(128'h13, 128'h12); #1;
    write_data = `combine(128'h15, 128'h14); #1;
    `assert(fifo_empty, 1'b0)
    `assert(fifo_full, 1'b1)

    write_en = 0; read_en = 1;
    `assert(read_data, 32'h0) #1;
    `assert(fifo_full, 1'b0)
    `assert(read_data, 32'h1) #1;
    `assert(read_data, 32'h2) #1;
    `assert(read_data, 32'h3) #1;
    `assert(read_data, 32'h4) #1;
    `assert(read_data, 32'h5) #1;
    `assert(read_data, 32'h6) #1;
    `assert(read_data, 32'h7) #1;
    `assert(read_data, 32'h8) #1;
    `assert(fifo_half_full, 1'b0)
    `assert(read_data, 32'h9) #1;
    `assert(read_data, 32'h10) #1;
    `assert(read_data, 32'h11) #1;
    `assert(read_data, 32'h12) #1;
    `assert(read_data, 32'h13) #1;
    `assert(read_data, 32'h14) #1;
    `assert(read_data, 32'h15) #1;
    `assert(fifo_empty, 1'b1)

    // Test 2: Concurrent read and write
    write_en = 1;
    write_data = `combine(128'h1, 128'h0); #1;

    read_en = 1;
    write_data = `combine(128'h3, 128'h2);
    `assert(read_data, 32'h0) #1;

    write_en = 0;
    `assert(read_data, 32'h1) #1;
    `assert(read_data, 32'h2) #1;
    `assert(read_data, 32'h3) #1;
    read_en = 0;

    `assert(fifo_empty, 1'b1)

    
    

    $finish;
    
end

endmodule
