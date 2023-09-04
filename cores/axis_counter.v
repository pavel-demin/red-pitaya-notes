
`timescale 1 ns / 1 ps

module axis_counter #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer CNTR_WIDTH = 32,
  parameter         CONTINUOUS = "FALSE"
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [CNTR_WIDTH-1:0]       cfg_data,

  // Master side
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid,
  input  wire                        m_axis_tready
);

  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg [CNTR_WIDTH-1:0] int_data_reg;
  reg int_enbl_reg, int_enbl_next;

  wire int_comp_wire, int_last_wire;

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
  assign int_last_wire = ~int_comp_wire;

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

        if(m_axis_tready & int_enbl_reg & int_comp_wire)
        begin
          int_cntr_next = int_cntr_reg + 1'b1;
        end

        if(m_axis_tready & int_enbl_reg & int_last_wire)
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

        if(m_axis_tready & int_enbl_reg & int_comp_wire)
        begin
          int_cntr_next = int_cntr_reg + 1'b1;
        end

        if(m_axis_tready & int_enbl_reg & int_last_wire)
        begin
          int_enbl_next = 1'b0;
        end
      end
    end
  endgenerate

  assign m_axis_tdata = int_cntr_reg;
  assign m_axis_tvalid = int_enbl_reg;

endmodule
