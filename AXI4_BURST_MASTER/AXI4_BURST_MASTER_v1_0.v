
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
		parameter  C_M00_AXI_TARGET_SLAVE_BASE_ADDR	= 40'hFFFC0000,
		parameter integer C_M00_AXI_BURST_LEN	    = 16,
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

    parameter MEMORY_TEST_COUNT = 13'd4096;

    wire m00_axi_init_axi_txn;
    wire [31:0] m_address;
    wire [31:0] m_data;
    wire [7:0]  m_burst_length;
    wire [1:0]  m_pg_mode;
    wire [1:0]  m_pg_seed;
    
    wire m00_axi_txn_done;
    wire m00_axi_error; 

    // Instantiation of Axi Bus Interface S00_AXI
	AXI4_BURST_MASTER_v1_0_S00_AXI # ( 
		.C_S_AXI_DATA_WIDTH(C_S00_AXI_DATA_WIDTH),
		.C_S_AXI_ADDR_WIDTH(C_S00_AXI_ADDR_WIDTH)
	) AXI4_BURST_MASTER_v1_0_S00_AXI_inst (
	    .init_txn(m00_axi_init_axi_txn),
	    .m_address(m_address),
	    .m_data(m_data),
	    .m_burst_length(m_burst_length),
        .m_pg_mode(m_pg_mode),
        .m_pg_seed(m_pg_seed),
        .txn_done(m00_axi_txn_done),
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

    reg pg_rst;
    reg pg_stall;
    wire [31:0] pattern_out;

    PATTERN_GEN PATTERN_GEN_inst (
        .clk(m00_axi_aclk),
        .rst(pg_rst),
        .stall(pg_stall),
        .pattern_sel(m_pg_mode),
        .seed(m_pg_seed),
        .pattern_out(pattern_out)
    );

    reg fifo_out_write_en;
    wire fifo_out_full;
    wire fifo_out_empty;

    FIFO FIFO_write_inst (
        .clk(m00_axi_aclk),
        .rst(m00_axi_init_axi_txn),
        .write_en(fifo_out_write_en),
        .write_data(pattern_out),
        .read(1'b0), // TODO
        .read_data(32'h00000000), // TODO
        .fifo_full(fifo_out_full), // TODO
        .fifo_empty(fifo_out_empty) // TODO
    );

    parameter [1:0] STATE_IDLE = 2'b00;
    parameter [1:0] STATE_WRITE = 2'b01;
    parameter [1:0] STATE_READ_COMPARE = 2'b10;
    parameter [1:0] STATE_UNUSED = 2'b11;
    reg [1:0] tester_state;
    reg [12:0] write_byte_counter;

    always @(posedge m00_axi_aclk) begin
        if (m00_axi_aresetn) begin
            tester_state <= STATE_IDLE;
            pg_rst <= 1'b1;
            pg_stall <= 1'b1;
        end else begin
            case (tester_state)
                STATE_IDLE: begin
                    // waits until m00_axi_init_axi_txn bit is set
                    pg_rst <= 1'b1;
                    if (m00_axi_init_axi_txn) begin
                        tester_state <= STATE_WRITE;
                    end
                    else begin
                        tester_state <= STATE_IDLE;
                    end
                end
                STATE_WRITE: begin
                    if (write_byte_counter == MEMORY_TEST_COUNT) begin
                        // sent 4096 bytes already, now read and compare
                        pg_rst <= 1'b1;
                        tester_state <= STATE_READ;
                    end
                    else begin
                        // clearing reset here -> first entry will be the seed
                        pg_rst <= 1'b0;
                        pg_stall <= 1'b0;
                        tester_state <= STATE_WRITE;

                        // feed FIFO with pattern
                        if (~fifo_out_full) begin
                            fifo_out_write_en <= 1'b1;
                            pg_stall <= 1'b0;
                            write_byte_counter <= write_byte_counter + 1'b1;
                        end
                        else begin
                            // FIFO is full, pause pattern generation
                            fifo_out_write_en <= 1'b0;
                            pg_stall <= 1'b1;
                            write_byte_counter <= write_byte_counter;
                        end
                    end
                end
                STATE_READ_COMPARE: begin
                    // TODO
                    if (compare done) begin
                        tester_state <= STATE_IDLE;
                        pg_rst <= 1'b1;
                    end
                    else begin
                        tester_state <= STATE_READ_COMPARE;
                        pg_rst <= 1'b0;
                    end

                end
                default: tester_state <= STATE_IDLE;
            endcase
        end
    end

    // TODO make adapter for FIFO to AXI4_BURST_MASTER_v1_0_M00_AXI_inst

// Instantiation of Axi Bus Interface M00_AXI
	AXI4_BURST_MASTER_v1_0_M00_AXI # ( 
		.C_M_TARGET_SLAVE_BASE_ADDR(C_M00_AXI_TARGET_SLAVE_BASE_ADDR),
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
		.TXN_DONE(m00_axi_txn_done),
		.ERROR(m00_axi_error),
        .m_address(m_address),
	    .m_data(m_data),
	    .m_burst_length(m_burst_length),
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
