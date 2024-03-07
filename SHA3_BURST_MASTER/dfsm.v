
`timescale 1 ns / 1 ps

module dfsm
	            (   clk,
	                reset,
	                keccak_data,
	                in_ready,
                    is_last,
	                byte_num,
	                buffer_full
	                
	             );

 input              clk;
 input              reset;
 
 input              buffer_full;
 output [63:0]      keccak_data;
 output [2:0]       byte_num;
 output             in_ready;
 output             is_last;              
  
 reg    [63:0]      keccak_data;
 reg     [2:0]      byte_num;
 reg                in_ready;
 reg                is_last;
              
 // NEED to I/O to BURST MASTER
 
 
 
 
 endmodule
