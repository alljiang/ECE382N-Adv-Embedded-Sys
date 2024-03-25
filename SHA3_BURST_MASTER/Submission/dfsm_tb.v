`timescale 1ns / 1ps

`define assert(signal, value) \
    if (signal !== value) begin \
        $display("ASSERTION FAILED in %m: signal != value"); \
        $finish; \
    end

`define combine(a, b) {a, b}

`define send_ocm_data(data2, data1) \
    while (init_master_txn !== 1) \
        #1; \
    read_active = 1; \
    read_done = 0; \
    #3; \
    ocm_data_out = `combine(data2, data1); \
    bus_data_valid = 1; \
    read_done = 1; \
    read_active = 0; \
    #1; \
    bus_data_valid = 0;

module dfsm_tb;

reg clk;
reg rst;

wire in_ready;
wire is_last;
wire read_data;
reg start;
reg [127:0] ocm_data_out;
reg bus_data_valid;
wire dfsm_read_ready;
wire [31:0] read_addr_index;
wire init_master_txn;
reg read_done;
reg read_active;
reg [15:0] number_bytes;
wire [511:0] keccak_hash_reg;
wire [31:0] debug1;
wire [31:0] debug2;
wire out_ready;

dfsm my_dfsm (
    .clk(clk),
    .reset(rst),

    .start(start),

    .keccak_in_ready(in_ready),
    .keccak_is_last(is_last),

    .ocm_data_out(ocm_data_out),
    .bus_data_valid(bus_data_valid),
    .dfsm_read_ready(dfsm_read_ready),
    .read_addr_index(read_addr_index),
    .init_master_txn(init_master_txn),
    .read_done(read_done),
    .read_active(read_active),
    
    .number_bytes(number_bytes),

    .keccak_hash_reg(keccak_hash_reg),
    .debug1(debug1),
    .debug2(debug2),
    .out_ready(out_ready)
);

initial begin
    clk = 1;
    forever #0.5 clk = ~clk;
end

initial begin

    rst = 1;
    start = 0;
    ocm_data_out = 128'b0;
    bus_data_valid = 0;
    read_done = 0;
    read_active = 0;
    number_bytes = 0;
    
    #5
    rst = 0;
    #5;
   
    start = 1;
    number_bytes = 16'd139;
    #1;

    `send_ocm_data("k brown ", "The quic");
    `send_ocm_data("s over t", "fox jump");
    `send_ocm_data("dog     ", "he lazy ");
    `send_ocm_data("k brown ", "The quic");
    `send_ocm_data("s over t", "fox jump");
    `send_ocm_data("dog     ", "he lazy ");
    `send_ocm_data("k brown ", "The quic");
    `send_ocm_data("s over t", "fox jump");
    `send_ocm_data("dog     ", "he lazy ");

    #50;

    $finish;
    
end

initial begin
  $dumpfile ("dfsm_tb.vcd");
  $dumpvars (0, my_dfsm);
  #1;
end

initial begin
   #200; // Wait a long time in simulation units (adjust as needed).
   $display("Caught by trap");
   $finish;
 end

endmodule
