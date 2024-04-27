# iverilog -o bus_fifo_tb bus_fifo_tb.v bus_fifo.v
# vvp bus_fifo_tb     
iverilog -o dfsm_tb dfsm_tb.v dfsm.v  bus_fifo.v ../AES/tiny_aes/aes_128.v ../AES/tiny_aes/aes_192.v ../AES/tiny_aes/aes_256.v ../AES/tiny_aes/round.v ../AES/tiny_aes/table.v
vvp dfsm_tb
