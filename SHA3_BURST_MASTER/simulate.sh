# iverilog -o bus_fifo_tb bus_fifo_tb.v bus_fifo.v
# vvp bus_fifo_tb     
iverilog -o dfsm_tb dfsm_tb.v dfsm.v  bus_fifo.v
vvp dfsm_tb