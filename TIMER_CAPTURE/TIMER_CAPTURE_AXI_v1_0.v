
`timescale 1 ns / 1 ps

	module TIMER_CAPTURE_AXI_v1_0 #
	(
		// Users to add parameters here

		// User parameters ends
		// Do not modify the parameters beyond this line


		// Parameters of Axi Slave Bus Interface S00_AXI
		parameter integer C_S00_AXI_DATA_WIDTH	= 32,
		parameter integer C_S00_AXI_ADDR_WIDTH	= 4
	)
	(
		// Users to add ports here

		// User ports ends
		// Do not modify the ports beyond this line


		// Ports of Axi Slave Bus Interface S00_AXI
		input wire  s00_axi_aclk,
		input wire  s00_axi_aresetn,
		input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_awaddr,
		input wire [2 : 0] s00_axi_awprot,
		input wire  s00_axi_awvalid,
		output wire  s00_axi_awready,
		input wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_wdata,
		input wire [(C_S00_AXI_DATA_WIDTH/8)-1 : 0] s00_axi_wstrb,
		input wire  s00_axi_wvalid,
		output wire  s00_axi_wready,
		output wire [1 : 0] s00_axi_bresp,
		output wire  s00_axi_bvalid,
		input wire  s00_axi_bready,
		input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_araddr,
		input wire [2 : 0] s00_axi_arprot,
		input wire  s00_axi_arvalid,
		output wire  s00_axi_arready,
		output wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_rdata,
		output wire [1 : 0] s00_axi_rresp,
		output wire  s00_axi_rvalid,
		input wire  s00_axi_rready
	);

    localparam RESET = 3'b111; 
    localparam COUNT = 3'b010; 
    localparam WAIT = 3'b011; 
    localparam IDLE = 3'b100;

    reg capture_gate_active;
    reg capture_complete;

    reg [2:0] state;
    reg [31:0] cap_timer_out;

    wire interrupt_enabled;
    wire timer_enabled;

    always @(posedge s00_axi_aclk) begin
        if (!s00_axi_aresetn) begin
            state <= RESET;
            cap_timer_out <= 32'b0;
        end 
        else begin
            case (state)
                RESET: begin
                    state <= IDLE;
                end
                IDLE: begin
                    cap_timer_out <= 32'b0;

                    if (!timer_enabled)
                        state <= IDLE;
                    else if (timer_enabled)
                        state <= COUNT;
                end
                COUNT: begin
                    cap_timer_out <= cap_timer_out + 1;

                    if (!timer_enabled || capture_gate)
                        // dma done signal
                        state <= WAIT;
                    else
                        state <= COUNT;
                end
                WAIT: begin
                    if (!timer_enabled)
                        state <= IDLE;
                    else
                        state <= WAIT;
                end
            endcase
        end
            
    end

    // dma monitoring logic
    always @(posedge s00_axi_aclk) begin
        if (!s00_axi_aresetn) begin
            capture_gate_active <= 1'b0;
            capture_complete <= 1'b0;
        end
        else begin
        end
    end

// Instantiation of Axi Bus Interface S00_AXI
	TIMER_CAPTURE_AXI_v1_0_S00_AXI # ( 
		.C_S_AXI_DATA_WIDTH(C_S00_AXI_DATA_WIDTH),
		.C_S_AXI_ADDR_WIDTH(C_S00_AXI_ADDR_WIDTH)
	) TIMER_CAPTURE_AXI_v1_0_S00_AXI_inst (
        .capture_gate(capture_gate_active),
        .capture_complete(capture_complete),
        .state(state),
        .cap_timer_out(cap_timer_out),
        .interrupt_enabled(interrupt_enabled),
        .timer_enabled(timer_enabled),

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

	// Add user logic here

	// User logic ends

	endmodule
