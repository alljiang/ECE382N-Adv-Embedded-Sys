
`timescale 1 ns / 1 ps

	module AXI4_BURST_MASTER_v1_0 #
	(
		// Users to add parameters here

		// User parameters ends
		// Do not modify the parameters beyond this line


		// Parameters of Axi Slave Bus Interface S00_AXI
		parameter integer C_S00_AXI_DATA_WIDTH	= 32,
		parameter integer C_S00_AXI_ADDR_WIDTH	= 5,

		// Parameters of Axi Master Bus Interface M00_AXI
		parameter integer C_M00_AXI_TARGET_SLAVE_BASE_ADDR = 40'hFFFC0000,
		parameter integer C_M00_AXI_BURST_LEN	    = 8,
		parameter integer C_M00_AXI_ID_WIDTH	    = 1,
		parameter integer C_M00_AXI_ADDR_WIDTH	    = 40,
		parameter integer C_M00_AXI_DATA_WIDTH	    = 32,
		parameter integer C_M00_AXI_AWUSER_WIDTH	= 0,
		parameter integer C_M00_AXI_ARUSER_WIDTH	= 0,
		parameter integer C_M00_AXI_WUSER_WIDTH	    = 0,
		parameter integer C_M00_AXI_RUSER_WIDTH	    = 0,
		parameter integer C_M00_AXI_BUSER_WIDTH	    = 0
	)
	(
		// Users to add ports here

		// User ports ends
		// Do not modify the ports beyond this line


		// Ports of Axi Slave Bus Interface S00_AXI
		input     wire                              s00_axi_aclk,
		input     wire                              s00_axi_aresetn,
		input     wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_awaddr,
		input     wire [2 : 0]                      s00_axi_awprot,
		input     wire                              s00_axi_awvalid,
		output    wire                              s00_axi_awready,
		input     wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_wdata,
		input     wire [(C_S00_AXI_DATA_WIDTH/8)-1:0] s00_axi_wstrb,
		input     wire                              s00_axi_wvalid,
		output    wire                              s00_axi_wready,
		output    wire [1 : 0]                      s00_axi_bresp,
		output    wire                              s00_axi_bvalid,
		input     wire                              s00_axi_bready,
		input     wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_araddr,
		input     wire [2 : 0]                      s00_axi_arprot,
		input     wire                              s00_axi_arvalid,
		output    wire                              s00_axi_arready,
		output    wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_rdata,
		output    wire [1 : 0]                      s00_axi_rresp,
		output    wire                              s00_axi_rvalid,
		input     wire                              s00_axi_rready,



		input     wire                                m00_axi_aclk,
		input     wire  m00_axi_aresetn,
		output    wire [C_M00_AXI_ID_WIDTH-1 : 0] m00_axi_awid,
		output    wire [C_M00_AXI_ADDR_WIDTH-1 : 0] m00_axi_awaddr,
		output    wire [7 : 0] m00_axi_awlen,
		output    wire [2 : 0] m00_axi_awsize,
		output    wire [1 : 0] m00_axi_awburst,
		output    wire  m00_axi_awlock,
		output    wire [3 : 0] m00_axi_awcache,
		output    wire [2 : 0] m00_axi_awprot,
		output    wire [3 : 0] m00_axi_awqos,
		output    wire [C_M00_AXI_AWUSER_WIDTH-1 : 0] m00_axi_awuser,
		output    wire  m00_axi_awvalid,
		input     wire  m00_axi_awready,
		output    wire [C_M00_AXI_DATA_WIDTH-1 : 0] m00_axi_wdata,
		output    wire [C_M00_AXI_DATA_WIDTH/8-1 : 0] m00_axi_wstrb,
		output    wire  m00_axi_wlast,
		output    wire [C_M00_AXI_WUSER_WIDTH-1 : 0] m00_axi_wuser,
		output    wire  m00_axi_wvalid,
		input     wire  m00_axi_wready,
		input     wire [C_M00_AXI_ID_WIDTH-1 : 0] m00_axi_bid,
		input     wire [1 : 0] m00_axi_bresp,
		input     wire [C_M00_AXI_BUSER_WIDTH-1 : 0] m00_axi_buser,
		input     wire  m00_axi_bvalid,
		output    wire  m00_axi_bready,
		output    wire [C_M00_AXI_ID_WIDTH-1 : 0] m00_axi_arid,
		output    wire [C_M00_AXI_ADDR_WIDTH-1 : 0] m00_axi_araddr,
		output    wire [7 : 0] m00_axi_arlen,
		output    wire [2 : 0] m00_axi_arsize,
		output wire [1 : 0] m00_axi_arburst,
		output wire  m00_axi_arlock,
		output wire [3 : 0] m00_axi_arcache,
		output wire [2 : 0] m00_axi_arprot,
		output wire [3 : 0] m00_axi_arqos,
		output wire [C_M00_AXI_ARUSER_WIDTH-1 : 0] m00_axi_aruser,
		output wire  m00_axi_arvalid,
		input wire  m00_axi_arready,
		input wire [C_M00_AXI_ID_WIDTH-1 : 0] m00_axi_rid,
		input wire [C_M00_AXI_DATA_WIDTH-1 : 0] m00_axi_rdata,
		input wire [1 : 0] m00_axi_rresp,
		input wire  m00_axi_rlast,
		input wire [C_M00_AXI_RUSER_WIDTH-1 : 0] m00_axi_ruser,
		input wire  m00_axi_rvalid,
		output wire  m00_axi_rready
	);

    localparam [12:0] MEMORY_TEST_COUNT = 13'd4096;

    wire m00_axi_init_axi_txn;
    wire [31:0] m_address;
    wire [31:0] m_data;
    wire [1:0]  pg_mode;
    wire [31:0]  pg_seed;

    // alljiang
    // wire m00_axi_txn_done;
    reg tester_done; 
    reg compare_mismatch_found;

    wire read_done;
    wire write_done;

    wire m00_axi_error;

    // Instantiation of Axi Bus Interface S00_AXI
	AXI4_BURST_MASTER_v1_0_S00_AXI # (
		.C_S_AXI_DATA_WIDTH(C_S00_AXI_DATA_WIDTH),
		.C_S_AXI_ADDR_WIDTH(C_S00_AXI_ADDR_WIDTH)
	) AXI4_BURST_MASTER_v1_0_S00_AXI_inst (
	    .init_txn(m00_axi_init_axi_txn),
	    .m_address(m_address),
        .pg_mode(pg_mode),
        .pg_seed(pg_seed),
        .compare_mismatch_found(compare_mismatch_found),
        .reads_done(read_done),
        .writes_done(write_done),
        .txn_done(tester_done),
        .txn_error(m00_axi_error),
		.S_AXI_ACLK(s00_axi_aclk),
		.S_AXI_ARESETN(s00_axi_aresetn),
		.S_AXI_AWADDR(s00_axi_awaddr),
		.S_AXI_AWPROT(s00_axi_awprot),
		.S_AXI_AWVALID(s00_axi_awvalid),
		.S_AXI_AWREADY(s00_axi_awready),
		.S_AXI_WDATA(s00_axi_wdata),
		.S_AXI_WSTRB(s00_axi_wstrb),
		.S_AXI_WVALID(s00_axi_wvalid),
		.S_AXI_WREADY(s00_axi_wready),
		.S_AXI_BRESP(s00_axi_bresp),
		.S_AXI_BVALID(s00_axi_bvalid),
		.S_AXI_BREADY(s00_axi_bready),
		.S_AXI_ARADDR(s00_axi_araddr),
		.S_AXI_ARPROT(s00_axi_arprot),
		.S_AXI_ARVALID(s00_axi_arvalid),
		.S_AXI_ARREADY(s00_axi_arready),
		.S_AXI_RDATA(s00_axi_rdata),
		.S_AXI_RRESP(s00_axi_rresp),
		.S_AXI_RVALID(s00_axi_rvalid),
		.S_AXI_RREADY(s00_axi_rready)
	);

    wire fifo_read_en;
    wire fifo_out_full;
    wire fifo_out_empty;
    
    wire [31:0] pattern_out;
    reg pg_rst;

    MY_FIFO2 #(.depth(8)) FIFO_write_inst (
        .clk(m00_axi_aclk),
        .rst(!m00_axi_aresetn), // TODO do proper reset
        .write_en(~fifo_out_full),
        .write_data(pattern_out),
        .read(fifo_read_en),
        .read_data(m_data),
        .fifo_full(fifo_out_full),
        .fifo_empty(fifo_out_empty)
    );

    PATTERN_GEN PATTERN_GEN_inst (
        .clk(m00_axi_aclk),
        .rst(pg_rst),
        .stall(fifo_out_full),
        .pattern_sel(pg_mode),
        .seed(pg_seed),
        .pattern_out(pattern_out)
    );

    wire fifo_in_write_en;
    reg fifo_in_read_en;
    wire fifo_in_full;
    wire fifo_in_empty;
    wire [31:0] fifo_in_read_data;
    wire [31:0] fifo_in_write_data;

    MY_FIFO2 #(.depth(8)) FIFO_read_inst (
        .clk(m00_axi_aclk),
        .rst(!m00_axi_aresetn),
        .write_en(fifo_in_write_en),
        .write_data(fifo_in_write_data),
        .read(fifo_in_read_en),
        .read_data(fifo_in_read_data),
        .fifo_full(fifo_in_full),
        .fifo_empty(fifo_in_empty)
    );

    reg pg_compare_rst;
    wire [31:0] pattern_compare_out;

    PATTERN_GEN PATTERN_GEN_compare_inst (
        .clk(m00_axi_aclk),
        .rst(pg_compare_rst),
        .stall(fifo_in_empty),
        .pattern_sel(pg_mode),
        .seed(pg_seed),
        .pattern_out(pattern_compare_out)
    );

    localparam [1:0] STATE_IDLE = 2'b00;
    localparam [1:0] STATE_WRITE_ACTIVE = 2'b01;
    localparam [1:0] STATE_AWAIT_COMPARE = 2'b10;
    reg [1:0] tester_state;
    reg [12:0] write_byte_counter;
    
    always @(posedge m00_axi_aclk) begin
        if (!m00_axi_aresetn) begin
            tester_state <= STATE_IDLE;
            write_byte_counter <= 13'd0;
            pg_rst <= 1'b1;
        end 
        else begin
            case (tester_state)
                STATE_IDLE: begin
                    // waits until m00_axi_init_axi_txn bit is set
                    if (m00_axi_init_axi_txn) begin
                        tester_state <= STATE_WRITE_ACTIVE;
                        tester_done <= 1'b0;
                        pg_rst <= 1'b0;
                    end
                    else begin
                        tester_state <= STATE_IDLE;
                        pg_rst <= 1'b1;
                    end
                end
                STATE_WRITE_ACTIVE: begin    
                    if (write_done)
                        tester_state <= STATE_AWAIT_COMPARE;
                    else
                        tester_state <= STATE_WRITE_ACTIVE;            
                end
                STATE_AWAIT_COMPARE: begin                    
                    // wait for read_done signal
                    if (read_done && fifo_in_empty) begin
                        tester_state <= STATE_IDLE;
                        tester_done <= 1'b1;
                    end
                    else begin
                        tester_state <= STATE_AWAIT_COMPARE;
                    end
                end
                default: tester_state <= STATE_IDLE;
            endcase
        end
    end

    reg [12:0] read_byte_counter;

    always @(posedge m00_axi_aclk) begin
        if (!m00_axi_aresetn) begin
            read_byte_counter <= 13'd0;
            pg_compare_rst <= 1'b1;
            compare_mismatch_found <= 1'b0;
        end
        else begin
            if (~fifo_in_empty) begin
                fifo_in_read_en <= 1'b1;
                read_byte_counter <= read_byte_counter + 1'b1;

                if (fifo_in_read_data != pattern_compare_out)
                    compare_mismatch_found <= 1'b1;
            end
            else begin
                fifo_in_read_en <= 1'b0;
                read_byte_counter <= read_byte_counter;
            end
        end
    end

// Instantiation of Axi Bus Interface M00_AXI
	AXI4_BURST_MASTER_v1_0_M00_AXI # (
		.C_M_AXI_BURST_LEN(C_M00_AXI_BURST_LEN),
		.C_M_AXI_ID_WIDTH(C_M00_AXI_ID_WIDTH),
		.C_M_AXI_ADDR_WIDTH(C_M00_AXI_ADDR_WIDTH),
		.C_M_AXI_DATA_WIDTH(C_M00_AXI_DATA_WIDTH),
		.C_M_AXI_AWUSER_WIDTH(C_M00_AXI_AWUSER_WIDTH),
		.C_M_AXI_ARUSER_WIDTH(C_M00_AXI_ARUSER_WIDTH),
		.C_M_AXI_WUSER_WIDTH(C_M00_AXI_WUSER_WIDTH),
		.C_M_AXI_RUSER_WIDTH(C_M00_AXI_RUSER_WIDTH),
		.C_M_AXI_BUSER_WIDTH(C_M00_AXI_BUSER_WIDTH)
	) AXI4_BURST_MASTER_v1_0_M00_AXI_inst (
		.INIT_AXI_TXN(m00_axi_init_axi_txn),
		// .TXN_DONE(m00_axi_txn_done),
		.ERROR(m00_axi_error),
        .m_address(m_address),
	    .m_data(m_data),
        .pg_fifo_read_en(fifo_read_en),
        .pg_fifo_full(fifo_out_full),

        .fifo_in_write_en(fifo_in_write_en),
        .fifo_in_write_data(fifo_in_write_data),
        .fifo_in_full(fifo_in_full),
        .read_done(read_done),
        .write_done(write_done),

		.M_AXI_ACLK(m00_axi_aclk),
		.M_AXI_ARESETN(m00_axi_aresetn),
		.M_AXI_AWID(m00_axi_awid),
		.M_AXI_AWADDR(m00_axi_awaddr),
		.M_AXI_AWLEN(m00_axi_awlen),
		.M_AXI_AWSIZE(m00_axi_awsize),
		.M_AXI_AWBURST(m00_axi_awburst),
		.M_AXI_AWLOCK(m00_axi_awlock),
		.M_AXI_AWCACHE(m00_axi_awcache),
		.M_AXI_AWPROT(m00_axi_awprot),
		.M_AXI_AWQOS(m00_axi_awqos),
		.M_AXI_AWUSER(m00_axi_awuser),
		.M_AXI_AWVALID(m00_axi_awvalid),
		.M_AXI_AWREADY(m00_axi_awready),
		.M_AXI_WDATA(m00_axi_wdata),
		.M_AXI_WSTRB(m00_axi_wstrb),
		.M_AXI_WLAST(m00_axi_wlast),
		.M_AXI_WUSER(m00_axi_wuser),
		.M_AXI_WVALID(m00_axi_wvalid),
		.M_AXI_WREADY(m00_axi_wready),
		.M_AXI_BID(m00_axi_bid),
		.M_AXI_BRESP(m00_axi_bresp),
		.M_AXI_BUSER(m00_axi_buser),
		.M_AXI_BVALID(m00_axi_bvalid),
		.M_AXI_BREADY(m00_axi_bready),
		.M_AXI_ARID(m00_axi_arid),
		.M_AXI_ARADDR(m00_axi_araddr),
		.M_AXI_ARLEN(m00_axi_arlen),
		.M_AXI_ARSIZE(m00_axi_arsize),
		.M_AXI_ARBURST(m00_axi_arburst),
		.M_AXI_ARLOCK(m00_axi_arlock),
		.M_AXI_ARCACHE(m00_axi_arcache),
		.M_AXI_ARPROT(m00_axi_arprot),
		.M_AXI_ARQOS(m00_axi_arqos),
		.M_AXI_ARUSER(m00_axi_aruser),
		.M_AXI_ARVALID(m00_axi_arvalid),
		.M_AXI_ARREADY(m00_axi_arready),
		.M_AXI_RID(m00_axi_rid),
		.M_AXI_RDATA(m00_axi_rdata),
		.M_AXI_RRESP(m00_axi_rresp),
		.M_AXI_RLAST(m00_axi_rlast),
		.M_AXI_RUSER(m00_axi_ruser),
		.M_AXI_RVALID(m00_axi_rvalid),
		.M_AXI_RREADY(m00_axi_rready)
	);

	// Add user logic here

	// User logic ends

	endmodule
	
	
module MY_FIFO2 #(parameter depth=8)
(
    input clk,
    input rst,
    input write_en,
    input [31:0] write_data,
    input read,
    output [31:0] read_data,
    output fifo_full,
    output fifo_empty
);
    
    wire [31:0] memory[7:0];
    // reg [31:0] memory[depth:0];
    reg [$clog2(depth)-1:0] write_ptr;
    reg [$clog2(depth)-1:0] read_ptr;
    reg [$clog2(depth):0] count;
    
    assign fifo_full = count == depth;
    assign fifo_empty = count == 0;
    assign read_data[31:0] = memory[read_ptr];

    assign memory[0] = 32'h12340000;
    assign memory[1] = 32'h12340001;
    assign memory[2] = 32'h12340002;
    assign memory[3] = 32'h12340003;
    assign memory[4] = 32'h12340004;
    assign memory[5] = 32'h12340005;
    assign memory[6] = 32'h12340006;
    assign memory[7] = 32'h12340007;
    
    always @(posedge clk) begin
        if (rst) begin
            count <= 0;
        end
        else begin
            case ({write_en, read})
            // assume FIFO will never be read while empty or written while full
                2'b00, 2'b11: count <= count;
                2'b01: count <= count - 1'b1;
                2'b10: count <= count + 1'b1;
            endcase
        end
    end
    
    always @(posedge clk) begin    
        if (rst) begin
            read_ptr <= 0;
        end
        else begin
            if (read & !fifo_empty) begin
                read_ptr <= read_ptr + 1'b1;
            end
        end
    
    end
    
    always @(posedge clk) begin
    
        if (rst) begin
            write_ptr <= 0;
        end
        else begin
            if (write_en & !fifo_full) begin
                write_ptr <= write_ptr + 1;
                // memory[write_ptr] <= write_data;
            end
        end
    
    end
    
endmodule

