# iverilog -o bus_fifo_tb bus_fifo_tb.v bus_fifo.v
# vvp bus_fifo_tb     
iverilog -o dfsm_tb dfsm_tb.v dfsm.v  bus_fifo.v keccak.v keccak/f_permutation.v keccak/padder.v keccak/padder1.v keccak/rconst2in1.v keccak/round2in1.v
vvp dfsm_tb
