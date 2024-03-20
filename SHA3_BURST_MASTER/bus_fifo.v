`timescale 1ns / 1ps

module Bus_FIFO #(parameter depth=32)
(
    input clk,
    input rst,
    input write_en,
    input [127:0] write_data,
    input read_en,
    output reg [63:0] read_data,
    output fifo_full,
    output fifo_half_full,
    output fifo_empty,
    output debug1
);

    reg [63:0] memory[depth-1:0];
    reg [$clog2(depth)-1:0] write_ptr;
    reg [$clog2(depth)-1:0] read_ptr;
    reg [$clog2(depth):0] count;
 
    assign debug1[31:0] = {
        8'hFF,
        3'b0, write_ptr,
        3'b0, read_ptr,
        2'b0, count
    }

    assign fifo_full = count == depth;
    assign fifo_half_full = count >= (depth >> 1);
    assign fifo_empty = count == 0;

    initial begin
        // all 1s
        read_data = 64'hAAAABBBBCCCCDDDD;
    end
    
    always @(posedge clk) begin
        if (rst) begin
            count <= 0;
        end
        else begin
            case ({write_en, read_en})
            // assume FIFO will never be read while empty or written while full
                2'b00: count <= count;
                2'b01: count <= count - 1;
                2'b10: count <= count + 2;
                2'b11: count <= count + 1;
            endcase
        end
    end
    
    always @(posedge clk) begin    
        if (rst) begin
            read_ptr <= 0;
        end
        else begin
            if (read_en & !fifo_empty) begin
                read_ptr <= read_ptr + 1'b1;
                read_data[63:0] <= memory[read_ptr];
            end
        end
    
    end
    
    always @(posedge clk) begin    
        if (rst) begin
            write_ptr <= 0;
        end
        else begin
            if (write_en & !fifo_full) begin
                write_ptr <= write_ptr + 2;
                memory[write_ptr] <= write_data[63:0];
                memory[write_ptr+1] <= write_data[127:64];
            end
        end
    
    end
    
endmodule
