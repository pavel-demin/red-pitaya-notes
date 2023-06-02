
`timescale 1 ns / 1 ps

module axis_trigger #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter         AXIS_TDATA_SIGNED = "FALSE"
)
(
  // System signals
  input  wire                        aclk,

  input  wire                        pol_data,
  input  wire [AXIS_TDATA_WIDTH-1:0] msk_data,
  input  wire [AXIS_TDATA_WIDTH-1:0] lvl_data,

  output wire                        trg_flag,

  // Slave side
  output wire                        s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid
);

  reg [1:0] int_comp_reg;
  wire int_comp_wire;

  generate
    if(AXIS_TDATA_SIGNED == "TRUE")
    begin : SIGNED
      assign int_comp_wire = $signed(s_axis_tdata & msk_data) >= $signed(lvl_data);
    end
    else
    begin : UNSIGNED
      assign int_comp_wire = (s_axis_tdata & msk_data) >= lvl_data;
    end
  endgenerate

  always @(posedge aclk)
  begin
    if(s_axis_tvalid)
    begin
      int_comp_reg <= {int_comp_reg[0], int_comp_wire};
    end
  end

  assign s_axis_tready = 1'b1;

  assign trg_flag = s_axis_tvalid & (pol_data ^ int_comp_reg[0]) & (pol_data ^ ~int_comp_reg[1]);

endmodule
