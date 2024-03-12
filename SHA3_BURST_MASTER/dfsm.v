
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

    input [127:0] ocm_data_out,
    input bus_data_valid,
    output dfsm_read_ready,
    output reg [31:0] read_addr_offset,
    output reg [511:0] keccak_hash_reg

);

    reg [15:0] hash_ptr;
    reg [3:0] state;

    reg fifo_read_en;
    wire [63:0] fifo_read_data;
    wire fifo_empty;

    Bus_FIFO #(64) bus_fifo (
        .clk(clk),
        .rst(reset),
        .write_data(ocm_data_out),
        .write_en(bus_data_valid),
        .read_en(fifo_read_en),
        .read_data(fifo_read_data),
        .fifo_half_full(~dfsm_read_ready),
        .fifo_empty(fifo_empty)
    );

    always @(posedge clk) begin
        if (reset) begin
            // all 1s using {} syntax
            keccak_hash_reg <= {64{8'b11000011}};
            hash_ptr <= 0;
            state <= 4'b0;
        end
        else begin
            // read data from bus_fifo, put into keccak_hash_regs
            case (state)
                4'b0: begin
                    if (~fifo_empty && hash_ptr < 8) begin
                        fifo_read_en <= 1;
                        state <= 4'b1;
                    end
                end
                4'b1: begin
                    keccak_hash_reg[(64*hash_ptr-1):64*hash_ptr] <= fifo_read_data;
                    hash_ptr <= hash_ptr + 1;
                    state <= 4'b0;
                    fifo_read_en <= 0;
                end
                
                default: begin
                end
            endcase
        end
    end

 endmodule
