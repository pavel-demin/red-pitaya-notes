
`timescale 1 ns / 1 ps

module axis_packetizer #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer CNTR_WIDTH = 32,
  parameter         CONTINUOUS = "FALSE",
  parameter         ALWAYS_READY = "FALSE"
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [CNTR_WIDTH-1:0]       cfg_data,

  // Slave side
  output wire                        s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid,

  // Master side
  input  wire                        m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid,
  output wire                        m_axis_tlast
);

  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg [CNTR_WIDTH-1:0] int_data_reg;
  reg int_enbl_reg, int_enbl_next;

  wire int_comp_wire, int_tvalid_wire, int_tlast_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_data_reg <= {(CNTR_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
    end
    else
    begin
      int_cntr_reg <= int_cntr_next;
      int_data_reg <= cfg_data;
      int_enbl_reg <= int_enbl_next;
    end
  end

  assign int_comp_wire = int_cntr_reg < int_data_reg;
  assign int_tvalid_wire = int_enbl_reg & s_axis_tvalid;
  assign int_tlast_wire = ~int_comp_wire;

  generate
    if(CONTINUOUS == "TRUE")
    begin : CONTINUE
      always @*
      begin
        int_cntr_next = int_cntr_reg;
        int_enbl_next = int_enbl_reg;

        if(~int_enbl_reg & int_comp_wire)
        begin
          int_enbl_next = 1'b1;
        end

        if(m_axis_tready & int_tvalid_wire & int_comp_wire)
        begin
          int_cntr_next = int_cntr_reg + 1'b1;
        end

        if(m_axis_tready & int_tvalid_wire & int_tlast_wire)
        begin
          int_cntr_next = {(CNTR_WIDTH){1'b0}};
        end
      end
    end
    else
    begin : STOP
      always @*
      begin
        int_cntr_next = int_cntr_reg;
        int_enbl_next = int_enbl_reg;

        if(~int_enbl_reg & int_comp_wire)
        begin
          int_enbl_next = 1'b1;
        end

        if(m_axis_tready & int_tvalid_wire & int_comp_wire)
        begin
          int_cntr_next = int_cntr_reg + 1'b1;
        end

        if(m_axis_tready & int_tvalid_wire & int_tlast_wire)
        begin
          int_enbl_next = 1'b0;
        end
      end
    end
  endgenerate

  generate
    if(ALWAYS_READY == "TRUE")
    begin : READY
      assign s_axis_tready = 1'b1;
    end
    else
    begin : BLOCKING
      assign s_axis_tready = int_enbl_reg & m_axis_tready;
    end
  endgenerate

  assign m_axis_tdata = s_axis_tdata;
  assign m_axis_tvalid = int_tvalid_wire;
  assign m_axis_tlast = int_enbl_reg & int_tlast_wire;

endmodule
