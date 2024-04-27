`timescale 1ns / 1ps

`define assert(signal, value) \
    if (signal !== value) begin \
        $display("ASSERTION FAILED in %m: signal != value"); \
        $finish; \
    end

`define combine(a, b) {a, b}

`define send_ocm_data(data) \
    while (init_master_txn !== 1) \
        #1; \
    read_active = 1; \
    read_done = 0; \
    #3; \
    ocm_data_out = data; \
    bus_data_valid = 1; \
    read_done = 1; \
    read_active = 0; \
    #1; \
    bus_data_valid = 0;

module dfsm_tb;

reg clk;
reg rst;

reg start;

reg [1:0] aes_key_size;
reg [255:0] aes_key;
reg [127:0] ctr_seed;

reg [127:0] ocm_data_out;
reg bus_data_valid;
wire dfsm_read_ready;
wire [31:0] read_addr_index;
wire init_master_txn;
reg read_done;
reg read_active;

reg [15:0] number_blocks;

wire [511:0] debug;
wire out_ready;

dfsm my_dfsm (
    .clk(clk),
    .reset(rst),

    .start(start),

    .aes_key_size(aes_key_size),
    .aes_key(aes_key),
    .ctr_seed(ctr_seed),

    .ocm_data_out(ocm_data_out),
    .bus_data_valid(bus_data_valid),
    .dfsm_read_ready(dfsm_read_ready),
    .read_addr_index(read_addr_index),
    .init_master_txn(init_master_txn),
    .read_done(read_done),
    .read_active(read_active),
    
    .number_blocks(number_blocks),

    .debug(debug),
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
    aes_key_size = 2'b00;
    aes_key = 256'b0;
    ctr_seed = 128'd10;

    bus_data_valid = 0;
    read_done = 0;
    read_active = 0;
    number_blocks = 0;
    
    #5
    rst = 0;
    #5;
   
    start = 1;
    number_blocks = 3;
    #1;

    `send_ocm_data("a");
    `send_ocm_data("b");
    `send_ocm_data("c");

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
