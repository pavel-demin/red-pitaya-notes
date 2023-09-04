
`timescale 1 ns / 1 ps

module axis_bram_reader #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer BRAM_DATA_WIDTH = 32,
  parameter integer BRAM_ADDR_WIDTH = 10,
  parameter         CONTINUOUS = "FALSE"
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [BRAM_ADDR_WIDTH-1:0]  cfg_data,
  output wire [BRAM_ADDR_WIDTH-1:0]  sts_data,

  // Master side
  output wire                        m_axis_tlast,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid,
  input  wire                        m_axis_tready,

  // BRAM port
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram CLK" *)
  output wire                        b_bram_clk,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram RST" *)
  output wire                        b_bram_rst,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram EN" *)
  output wire                        b_bram_en,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram ADDR" *)
  output wire [BRAM_ADDR_WIDTH-1:0]  b_bram_addr,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram DOUT" *)
  input  wire [BRAM_DATA_WIDTH-1:0]  b_bram_rdata
);

  reg [BRAM_ADDR_WIDTH-1:0] int_addr_reg, int_addr_next;
  reg [BRAM_ADDR_WIDTH-1:0] int_data_reg;
  reg int_enbl_reg, int_enbl_next;

  wire [AXIS_TDATA_WIDTH-1:0] int_data_wire;
  wire [2:0] int_last_wire, int_valid_wire, int_ready_wire;
  wire int_comp_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_addr_reg <= {(BRAM_ADDR_WIDTH){1'b0}};
      int_data_reg <= {(BRAM_ADDR_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
    end
    else
    begin
      int_addr_reg <= int_addr_next;
      int_data_reg <= cfg_data;
      int_enbl_reg <= int_enbl_next;
    end
  end

  assign int_comp_wire = int_addr_reg < int_data_reg;

  assign int_last_wire[0] = ~int_comp_wire;
  assign int_valid_wire[0] = int_enbl_reg;

  generate
    if(CONTINUOUS == "TRUE")
    begin : CONTINUE
      always @*
      begin
        int_addr_next = int_addr_reg;
        int_enbl_next = int_enbl_reg;

        if(~int_enbl_reg & int_comp_wire)
        begin
          int_enbl_next = 1'b1;
        end

        if(int_ready_wire[0] & int_enbl_reg & int_comp_wire)
        begin
          int_addr_next = int_addr_reg + 1'b1;
        end

        if(int_ready_wire[0] & int_enbl_reg & int_last_wire[0])
        begin
          int_addr_next = {(BRAM_ADDR_WIDTH){1'b0}};
        end
      end
    end
    else
    begin : STOP
      always @*
      begin
        int_addr_next = int_addr_reg;
        int_enbl_next = int_enbl_reg;

        if(~int_enbl_reg & int_comp_wire)
        begin
          int_enbl_next = 1'b1;
        end

        if(int_ready_wire[0] & int_enbl_reg & int_comp_wire)
        begin
          int_addr_next = int_addr_reg + 1'b1;
        end

        if(int_ready_wire[0] & int_enbl_reg & int_last_wire[0])
        begin
          int_enbl_next = 1'b0;
        end
      end
    end
  endgenerate

  output_buffer #(
    .DATA_WIDTH(1)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_last_wire[0]), .in_valid(int_valid_wire[0]), .in_ready(int_ready_wire[0]),
    .out_data(int_last_wire[1]), .out_valid(int_valid_wire[1]), .out_ready(int_ready_wire[1])
  );

  input_buffer #(
    .DATA_WIDTH(AXIS_TDATA_WIDTH + 1)
  ) buf_1 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data({int_last_wire[1], b_bram_rdata}),
    .in_valid(int_valid_wire[1]), .in_ready(int_ready_wire[1]),
    .out_data({int_last_wire[2], int_data_wire}),
    .out_valid(int_valid_wire[2]), .out_ready(int_ready_wire[2])
  );

  output_buffer #(
    .DATA_WIDTH(AXIS_TDATA_WIDTH + 1)
  ) buf_2 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data({int_last_wire[2], int_data_wire}),
    .in_valid(int_valid_wire[2]), .in_ready(int_ready_wire[2]),
    .out_data({m_axis_tlast, m_axis_tdata}),
    .out_valid(m_axis_tvalid), .out_ready(m_axis_tready)
  );

  assign sts_data = int_addr_reg - int_valid_wire[1] - int_valid_wire[2] - m_axis_tvalid;

  assign b_bram_clk = aclk;
  assign b_bram_rst = ~aresetn;
  assign b_bram_en = int_ready_wire[0];
  assign b_bram_addr = int_addr_reg;

endmodule
