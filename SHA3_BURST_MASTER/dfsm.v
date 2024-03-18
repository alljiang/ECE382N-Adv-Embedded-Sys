
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
    input start,

    input [127:0] ocm_data_out,
    input bus_data_valid,
    output dfsm_read_ready,
    output reg [31:0] read_addr_index,
    output reg init_master_txn,
    input wire read_done,
    output reg [511:0] keccak_hash_reg,
    output wire [64*8-1:0] debug_memory

);

    reg [4:0] state;

    reg fifo_read_en;
    wire [63:0] fifo_read_data;
    wire fifo_empty;
    wire fifo_half_full;
    wire fifo_full;

    Bus_FIFO #(64) bus_fifo (
        .clk(clk),
        .rst(reset),
        .write_data(ocm_data_out),
        .write_en(bus_data_valid),
        .read_en(fifo_read_en),
        .debug_memory(debug_memory),
        .read_data(fifo_read_data),
        .fifo_full(fifo_full),
        .fifo_half_full(fifo_half_full),
        .fifo_empty(fifo_empty)
    );

    assign dfsm_read_ready = ~fifo_half_full;

    reg [1:0] read_state;

    always @(posedge clk) begin
        if (reset) begin
           read_addr_index <= 0;
           read_state <= 2'b10;
           init_master_txn <= 0;
        end
        else begin
            case (read_state)
                2'b0: begin
                    if (read_addr_index < 1) begin
                        init_master_txn <= 1;
                        read_state <= 2'b1;
                    end
                end
                2'b1: begin
                    init_master_txn <= 0;
                    if (read_done) begin
                        read_state <= 2'b10;
                        read_addr_index <= read_addr_index + 1;
                    end
                end
                2'b10: begin
                    if (start) begin
                        read_state <= 1'b0;
                    end
                end
                default: begin
                end
            endcase
        end
    end

    always @(posedge clk) begin
        if (reset) begin
            // all 1s using {} syntax
            keccak_hash_reg <= {64{8'b11000011}};
            state <= 5'd0;
            fifo_read_en <= 0;
        end
        else begin
            // read data from bus_fifo, put into keccak_hash_regs
            case (state)
                5'd0: begin
                    if (~fifo_empty) begin
                        fifo_read_en <= 1;
                        state <= 5'd1;
                    end
                end
                5'd1: begin
                    fifo_read_en <= 0;
                    state <= 5'd2;
                end
                5'd2: begin
                    keccak_hash_reg[63:0] <= fifo_read_data;
                    state <= 5'd3;
                end
                5'd3: begin
                    if (~fifo_empty) begin
                        fifo_read_en <= 1;
                        state <= 5'd4;
                    end
                end
                5'd4: begin
                    fifo_read_en <= 0;
                    state <= 5'd5;
                end
                5'd5: begin
                    keccak_hash_reg[127:64] <= fifo_read_data;
                    state <= 5'd6;
                end
                5'd6: begin
                    if (~fifo_empty) begin
                        fifo_read_en <= 1;
                        state <= 5'd7;
                    end
                end
                5'd7: begin
                    fifo_read_en <= 0;
                    state <= 5'd8;
                end
                5'd8: begin
                    keccak_hash_reg[191:128] <= fifo_read_data;
                    state <= 5'd9;
                end
                5'd9: begin
                    if (~fifo_empty) begin
                        fifo_read_en <= 1;
                        state <= 5'd10;
                    end
                end
                5'd10: begin
                    fifo_read_en <= 0;
                    state <= 5'd11;
                end
                5'd11: begin
                    keccak_hash_reg[255:192] <= fifo_read_data;
                    state <= 5'd12;
                end
                5'd12: begin
                    if (~fifo_empty) begin
                        fifo_read_en <= 1;
                        state <= 5'd13;
                    end
                end
                5'd13: begin
                    fifo_read_en <= 0;
                    state <= 5'd14;
                end
                5'd14: begin
                    keccak_hash_reg[319:256] <= fifo_read_data;
                    state <= 5'd15;
                end
                5'd15: begin
                    if (~fifo_empty) begin
                        fifo_read_en <= 1;
                        state <= 5'd16;
                    end
                end
                5'd16: begin
                    fifo_read_en <= 0;
                    state <= 5'd17;
                end
                5'd17: begin
                    keccak_hash_reg[383:320] <= fifo_read_data;
                    state <= 5'd18;
                end
                5'd18: begin
                    if (~fifo_empty) begin
                        fifo_read_en <= 1;
                        state <= 5'd19;
                    end
                end
                5'd19: begin
                    fifo_read_en <= 0;
                    state <= 5'd20;
                end
                5'd20: begin
                    keccak_hash_reg[447:384] <= fifo_read_data;
                    state <= 5'd21;
                end
                5'd21: begin
                    if (~fifo_empty) begin
                        fifo_read_en <= 1;
                        state <= 5'd22;
                    end
                end
                5'd22: begin
                    fifo_read_en <= 0;
                    state <= 5'd23;
                end
                5'd23: begin
                    keccak_hash_reg[511:448] <= fifo_read_data;
                    state <= 5'd24;
                end
                5'd24: begin
                end

                default: begin
                end
            endcase
        end
    end

 endmodule
