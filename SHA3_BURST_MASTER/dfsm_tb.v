`timescale 1ns / 1ps

`define assert(signal, value) \
    if (signal !== value) begin \
        $display("ASSERTION FAILED in %m: signal != value"); \
        $finish; \
    end

`define combine(a, b) {a, b}

module dfsm_tb;

reg clk;
reg rst;

wire [63:0] keccak_input;
wire in_ready;
wire is_last;
wire read_data;
wire [2:0] byte_num;
reg buffer_full;
reg start;
reg [127:0] ocm_data_out;
reg bus_data_valid;
wire dfsm_read_ready;
wire [31:0] read_addr_index;
wire init_master_txn;
reg read_done;
wire [511:0] keccak_hash_reg;
wire [64*8-1:0] debug_memory;

my_dfsm #(16) dfsm (
    .clk(clk),
    .reset(rst),
    .keccak_input(keccak_input),
    .in_ready(in_ready),
    .is_last(is_last),
    .read_data(read_data),
    .byte_num(byte_num),
    .buffer_full(buffer_full),
    .start(start),

    .ocm_data_out(ocm_data_out),
    .bus_data_valid(bus_data_valid),
    .dfsm_read_ready(dfsm_read_ready),
    .read_addr_index(read_addr_index),
    .init_master_txn(init_master_txn),
    .read_done(read_done),
    .keccak_hash_reg(keccak_hash_reg),
    .debug_memory(debug_memory)
);

initial begin
    clk = 1;
    forever #0.5 clk = ~clk;
end

initial begin

    rst = 1;
    buffer_full = 0;
    start = 0;
    ocm_data_out = 128'b0;
    bus_data_valid = 0;
    read_done = 0;
    
    #5
    rst = 0;
    #1;
   

    $finish;
    
end

initial begin
  $dumpfile ("dfsm.vcd");
  $dumpvars (0, my_dfsm);
  #1;
end

endmodule
