
`timescale 1 ns / 1 ps

`define AES_128 2'b00
`define AES_192 2'b01
`define AES_256 2'b10

module dfsm (   
    input clk,
    input reset,

    input start,

    input wire [1:0] aes_key_size,
    input wire [255:0] aes_key,
    input wire [127:0] ctr_iv,

    input [127:0] ocm_data_out,
    input bus_data_valid,
    output dfsm_read_ready,
    output reg [31:0] read_addr_index,
    output reg init_master_txn,
    input wire read_done,
    input read_active,

    input wire output_fifo_read_en,
    output wire [127:0] output_fifo_read_data,
    output wire output_fifo_empty,
    input wire write_finished,

    input wire [15:0] number_blocks,

    output wire [191:0] debug,
    output wire out_ready
);

    reg [3:0] state;
    
    wire [127:0] aes_128_out;
    reg [127:0] aes_128_in;

    aes_128 aes_128_top (
        .clk(clk),
        .state(aes_128_in[127:0]),
        .key(aes_key[127:0]),
        .out(aes_128_out[127:0])
    );

    reg fifo_read_en;
    wire [127:0] fifo_read_data;
    wire fifo_empty;
    wire fifo_half_full;
    wire fifo_full;

    Bus_FIFO #(16) bus_fifo (
        .clk(clk),
        .rst(reset),
        .write_data(ocm_data_out),
        .write_en(bus_data_valid),
        .read_en(fifo_read_en),
        .read_data(fifo_read_data),
        .fifo_full(fifo_full),
        .fifo_half_full(fifo_half_full),
        .fifo_empty(fifo_empty)
    );

    reg aes_fifo_write_en;
    wire aes_fifo_full;
    wire aes_fifo_half_full;
    wire aes_fifo_empty;
    reg aes_fifo_read_en;
    wire [127:0] aes_fifo_read_data;

    Bus_FIFO #(16) aes_fifo (
        .clk(clk),
        .rst(reset),
        .write_data(fifo_read_data),
        .write_en(aes_fifo_write_en),
        .read_en(aes_fifo_read_en),
        .read_data(aes_fifo_read_data),
        .fifo_full(aes_fifo_full),
        .fifo_half_full(aes_fifo_half_full),
        .fifo_empty(aes_fifo_empty)
    );

    reg output_fifo_write_en;

    Bus_FIFO #(16) output_fifo (
        .clk(clk),
        .rst(reset),
        .write_data(aes_fifo_read_data[127:0] ^ aes_128_out[127:0]),
        .write_en(output_fifo_write_en),
        .read_en(output_fifo_read_en),
        .read_data(output_fifo_read_data[127:0]),
        .fifo_full(output_fifo_full),
        .fifo_half_full(output_fifo_half_full),
        .fifo_empty(output_fifo_empty)
    );

    assign dfsm_read_ready = ~fifo_half_full;

    reg [1:0] read_state;
    reg [15:0] blocks_to_read;
    reg [31:0] bus_fifo_count;

    // this state machine will read words from the bus_fifo
    always @(posedge clk) begin
        if (reset) begin
            read_addr_index <= 0;
            read_state <= 2'b11;
            init_master_txn <= 0;
            blocks_to_read <= 0;
            bus_fifo_count <= 0;
        end
        else begin
            case (read_state)
                2'b00: begin
                    if (blocks_to_read > 0) begin
                        init_master_txn <= 1;
                        blocks_to_read <= blocks_to_read - 1;
                        bus_fifo_count <= bus_fifo_count + 1;

                        read_state <= 2'b01;
                    end
                    else
                        read_state <= 2'b00;
                end
                2'b01: begin
                    init_master_txn <= 0;
                    if (read_active) begin
                        read_state <= 2'b10;
                    end
                    else
                        read_state <= 2'b01;
                end
                2'b10: begin
                    if (read_done) begin
                        read_state <= 2'b00;
                        read_addr_index <= read_addr_index + 1;
                        bus_fifo_count <= bus_fifo_count + 1;
                    end
                    else
                        read_state <= 2'b10;
                end
                2'b11: begin
                    if (start) begin
                        read_state <= 2'b00;
                        blocks_to_read <= number_blocks;
                    end
                    else
                        read_state <= 2'b11;
                end
                default: begin
                end
            endcase
        end
    end

    reg [15:0] blocks_to_process;
    reg [28:0] delay_pipe;

    // series of flip-flops for counting delays
    genvar i;
    for (i = 1; i < 29; i = i + 1) begin
        always @(posedge clk) begin
            if (reset) begin
                delay_pipe[i] <= 0;
            end
            else begin
                delay_pipe[i] <= delay_pipe[i-1];
            end
        end
    end

    reg [31:0] output_fifo_count;
    // aes output handling
    always @(posedge clk) begin
        if (reset) begin
            aes_fifo_read_en <= 0;
            output_fifo_write_en <= 0;
            output_fifo_count <= 32'b0;
        end
        else begin
            if (aes_key_size == `AES_128) begin
                if (delay_pipe[19] == 1) begin
                    aes_fifo_read_en <= 1;
                end
                else if (delay_pipe[20] == 1) begin
                    aes_fifo_read_en <= 0;
                    output_fifo_write_en <= 1;
                    output_fifo_count <= output_fifo_count + 1;
                end
                else if (delay_pipe[21] == 1) begin
                    output_fifo_write_en <= 0;
                end
                else begin
                    aes_fifo_read_en <= 0;              
                end
            end
        end
    end

    reg [127:0] counter;

    reg [31:0] aes_fifo_count;

    // aes fifo
    always @(posedge clk) begin
        if (reset) begin
            state <= 4'd0;
            fifo_read_en <= 0;
            blocks_to_process <= 0;
            delay_pipe[0] <= 0;
            aes_128_in <= 0;
            aes_fifo_write_en <= 0;
            counter <= 0;
            aes_fifo_count <= 0;
        end
        else begin
            case (state)
                4'd0: begin
                    // wait for start signal
                    if (start) begin
                        state <= 4'd1;
                        aes_128_in <= 0;
                        blocks_to_process <= number_blocks;
                    end
                end
                4'd1: begin
                    if (~fifo_empty && ~aes_fifo_full && blocks_to_process > 0) begin
                        fifo_read_en <= 1;
                        blocks_to_process <= blocks_to_process - 1;
                        aes_fifo_count <= aes_fifo_count + 1;

                        state <= 4'd2;
                    end
                    else if (blocks_to_process == 0) begin
                        state <= 4'd5;
                    end
                    else begin
                        state <= 4'd1;
                    end
                end
                4'd2: begin
                    // start delay pipe to wait for aes_128 to finish
                    aes_128_in <= ctr_iv + counter;
                    counter <= counter + 1;
                    delay_pipe[0] <= 1;

                    // move from bus fifo to aes fifo
                    fifo_read_en <= 0;
                    aes_fifo_write_en <= 1;
                    state <= 4'd3;
                end
                4'd3: begin
                    aes_128_in <= 0;
                    delay_pipe[0] <= 0;

                    aes_fifo_write_en <= 0;
                    state <= 4'd1;
                end
                4'd5: begin
                    // done
                end
                default: begin
                end
            endcase
        end
    end

    assign debug[95:64] = output_fifo_count;
    assign debug[63:32] = aes_fifo_count;
    assign debug[31:0] = bus_fifo_count;
    // assign debug[127:96] = {31'b0, read_done};
    // assign debug[95:64] = {31'b0, read_active};
    // assign debug[63:32] = {30'b0, read_state};
    // assign debug[30:0] = blocks_to_read;
    // assign debug[31] = 1'b1;

    assign out_ready = write_finished && output_fifo_empty && aes_fifo_empty && fifo_empty && blocks_to_read == 0 && blocks_to_process == 0;

 endmodule