
`timescale 1 ns / 1 ps

module dfsm (   
    input clk,
    input reset,

    input start,

    output keccak_in_ready,
    output keccak_is_last,

    input [127:0] ocm_data_out,
    input bus_data_valid,
    output dfsm_read_ready,
    output reg [31:0] read_addr_index,
    output reg init_master_txn,
    input wire read_done,
    input read_active,

    input wire [15:0] number_bytes,

    output wire [511:0] keccak_hash_reg,
    output wire [31:0] debug1,
    output wire [31:0] debug2,
    output wire [64*8-1:0] memory_debug,
    output wire out_ready
);

    // Keccak I/O
    reg in_ready;
    reg is_last;
    reg [2:0] byte_num;
    wire buffer_full;
    assign keccak_in_ready = in_ready;
    assign keccak_is_last = is_last;

    reg [3:0] state;

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
        .read_data(fifo_read_data),
        .fifo_full(fifo_full),
        .fifo_half_full(fifo_half_full),
        .fifo_empty(fifo_empty),
        .debug1(debug1),
        .memory_debug(memory_debug)
    );

   keccak KECCAK_TOP( 
        .clk(clk),
        .reset(reset),
        .in(fifo_read_data),
        .in_ready(in_ready),
        .is_last(is_last),
        .byte_num(byte_num),
        .buffer_full(buffer_full),
        .out(keccak_hash_reg),
        .out_ready(out_ready)
    );

    assign dfsm_read_ready = ~fifo_half_full;

    reg [1:0] read_state;
    reg [15:0] bytes_to_read;
    reg [15:0] test_count;

    // this state machine will read words from the bus_fifo
    always @(posedge clk) begin
        if (reset) begin
            read_addr_index <= 0;
            read_state <= 2'b11;
            init_master_txn <= 0;
            bytes_to_read <= 0;
        end
        else begin
            case (read_state)
                2'b00: begin
                    if (bytes_to_read > 0) begin
                        init_master_txn <= 1;

                        if (bytes_to_read <= 16)
                            bytes_to_read <= 0;
                        else
                            bytes_to_read <= bytes_to_read - 16;

                        read_state <= 2'b01;
                    end
                    else
                        read_state <= 2'b00;
                end
                2'b01: begin
                    init_master_txn <= 0;
                    if (read_active)
                        read_state <= 2'b10;
                    else
                        read_state <= 2'b01;
                end
                2'b10: begin
                    if (read_done) begin
                        read_state <= 2'b00;
                        read_addr_index <= read_addr_index + 1;
                    end
                    else
                        read_state <= 2'b10;
                end
                2'b11: begin
                    if (start) begin
                        read_state <= 2'b00;
                        bytes_to_read <= number_bytes;
                    end
                    else
                        read_state <= 2'b11;
                end
                default: begin
                end
            endcase
        end
    end

    reg [15:0] bytes_to_process;

    always @(posedge clk) begin
        if (reset) begin
            state <= 4'd0;
            fifo_read_en <= 0;
            bytes_to_process <= 0;
            in_ready <= 0;
            is_last <= 0;
            byte_num <= 0;
            test_count <= 0;
        end
        else begin
            case (state)
                4'd0: begin
                    // wait for start signal
                    if (start) begin
                        state <= 4'd1;
                        bytes_to_process <= number_bytes;
                    end
                end
                4'd1: begin
                    if (~fifo_empty && bytes_to_process > 0 && ~buffer_full) begin
                        fifo_read_en <= 1;
                        test_count <= test_count + 1;

                        state <= 4'd2;
                    end
                end
                4'd2: begin
                    fifo_read_en <= 0;
                    state <= 4'd3;
                    in_ready <= 1;

                    if (bytes_to_process <= 8) begin
                        is_last <= 1;
                    end

                    if (bytes_to_process >= 8) begin
                        bytes_to_process <= bytes_to_process - 8;
                        byte_num <= 0;
                    end
                    else begin
                        bytes_to_process <= 0;
                        byte_num <= bytes_to_process;
                    end
                end
                4'd3: begin
                    in_ready <= 0;
                    
                    if (is_last) begin
                        state <= 4'd4;
                        is_last <= 0;
                    end
                    else begin
                        state <= 4'd1;
                    end
                end
                4'd4: begin

                    // wait for out_ready
                    if (out_ready) begin
                        state <= 4'd5;
                    end
                end
                4'd5: begin
                    // done
                end
                default: begin
                end
            endcase
        end
    end

    // assign debug1[31:0] = {
    //     bytes_to_read,
    //     test_count
    // };

    assign debug2[31:0] = {
        3'b0, fifo_empty,
        3'b0, fifo_read_en,
        3'b0, read_active,
        3'b0, read_done,

        3'b0, in_ready,
        3'b0, is_last,
        1'b0, byte_num,
        state
    };

 endmodule
