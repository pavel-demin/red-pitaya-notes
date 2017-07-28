
`timescale 1 ns / 1 ps

module axis_lfsr #
(
  parameter integer AXIS_TDATA_WIDTH = 64,
  parameter         HAS_TREADY = "FALSE"
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  // Master side
  input  wire                        m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  reg [AXIS_TDATA_WIDTH-1:0] int_lfsr_reg, int_lfsr_next;
  reg int_enbl_reg, int_enbl_next;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_lfsr_reg <= 64'h5555555555555555;
      int_enbl_reg <= 1'b0;
    end
    else
    begin
      int_lfsr_reg <= int_lfsr_next;
      int_enbl_reg <= int_enbl_next;
    end
  end

  generate
    if(HAS_TREADY == "TRUE")
    begin : HAS_TREADY
      always @*
      begin
        int_lfsr_next = int_lfsr_reg;
        int_enbl_next = int_enbl_reg;

        if(~int_enbl_reg)
        begin
          int_enbl_next = 1'b1;
        end

        if(int_enbl_reg & m_axis_tready)
        begin
          int_lfsr_next = {int_lfsr_reg[62:0], int_lfsr_reg[62] ~^ int_lfsr_reg[61]};
        end
      end
    end
    else
    begin : NO_TREADY
      always @*
      begin
        int_lfsr_next = int_lfsr_reg;
        int_enbl_next = int_enbl_reg;

        if(~int_enbl_reg)
        begin
          int_enbl_next = 1'b1;
        end

        if(int_enbl_reg)
        begin
          int_lfsr_next = {int_lfsr_reg[62:0], int_lfsr_reg[62] ~^ int_lfsr_reg[61]};
        end
      end
    end
  endgenerate

  assign m_axis_tdata = int_lfsr_reg;
  assign m_axis_tvalid = int_enbl_reg;

endmodule
