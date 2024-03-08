
`timescale 1 ns / 1 ps

module dfsm (   
    input clk,
    input reset,

    // keccak I/O
    output reg [63:0] keccak_input,
    output reg in_ready,
    output reg is_last,
    output reg [2:0] byte_num,
    input buffer_full,

    // burst master I/O
    output reg fifo_read_en;
    input [63:0] fifo_read_data;
    output fifo_empty;
    
);

 // NEED to I/O to BURST MASTER
 
    
 
 
 endmodule
