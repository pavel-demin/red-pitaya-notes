
`timescale 1 ns / 1 ps

module axis_pulse_height_analyzer #
(
  parameter integer AXIS_TDATA_WIDTH = 16,
  parameter         AXIS_TDATA_SIGNED = "FALSE",
  parameter integer CNTR_WIDTH = 16
)
(
  // System signals
  input  wire                           aclk,
  input  wire                           aresetn,

  input  wire [CNTR_WIDTH-1:0]          cfg_data,
  input  wire [AXIS_TDATA_WIDTH-1:0]    min_data,
  input  wire [AXIS_TDATA_WIDTH-1:0]    max_data,

  // Slave side
  output wire                           s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0]    s_axis_tdata,
  input  wire                           s_axis_tvalid,

  // Master side
  input  wire                           m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0]    m_axis_tdata,
  output wire                           m_axis_tvalid
);

  reg [AXIS_TDATA_WIDTH-1:0] int_data_reg[1:0];
  reg [AXIS_TDATA_WIDTH-1:0] int_min_reg;
  reg [CNTR_WIDTH-1:0] int_cntr_reg;
  reg int_enbl_reg;
  reg int_rising_reg;

  wire [AXIS_TDATA_WIDTH-1:0] int_data_wire;
  wire int_mincut_wire, int_maxcut_wire, int_rising_wire, int_delay_wire, int_valid_wire;

  assign int_delay_wire = int_cntr_reg < cfg_data;
  assign int_valid_wire = s_axis_tvalid & int_enbl_reg & int_rising_reg & ~int_rising_wire & int_mincut_wire & int_maxcut_wire;

  generate
    if(AXIS_TDATA_SIGNED == "TRUE")
    begin : SIGNED
      assign int_rising_wire = $signed(int_data_reg[1]) < $signed(s_axis_tdata);
      assign int_data_wire = $signed(int_data_reg[0]) - $signed(int_min_reg);
      assign int_mincut_wire = $signed(int_data_wire) > $signed(min_data);
      assign int_maxcut_wire = $signed(int_data_wire) < $signed(max_data);
    end
    else
    begin : UNSIGNED
      assign int_rising_wire = int_data_reg[1] < s_axis_tdata;
      assign int_data_wire = int_data_reg[0] - int_min_reg;
      assign int_mincut_wire = int_data_wire > min_data;
      assign int_maxcut_wire = int_data_wire < max_data;
    end
  endgenerate

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_data_reg[0] <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_data_reg[1] <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_min_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
      int_rising_reg <= 1'b0;
    end
    else if(s_axis_tvalid & s_axis_tready)
    begin
      int_data_reg[0] <= s_axis_tdata;
      int_data_reg[1] <= int_data_reg[0];
      int_rising_reg <= int_rising_wire;

      if(int_delay_wire)
      begin
        int_cntr_reg <= int_cntr_reg + 1'b1;
      end

      // minimum after delay
      if(~int_delay_wire & ~int_rising_reg & int_rising_wire)
      begin
        int_min_reg <= int_data_reg[1];
        int_enbl_reg <= 1'b1;
      end

      // maximum after minimum
      if(int_enbl_reg & int_rising_reg & ~int_rising_wire & int_mincut_wire)
      begin
        int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
        int_enbl_reg <= 1'b0;
      end
    end
  end

  output_buffer #(
    .DATA_WIDTH(AXIS_TDATA_WIDTH)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_data_wire), .in_valid(int_valid_wire), .in_ready(s_axis_tready),
    .out_data(m_axis_tdata), .out_valid(m_axis_tvalid), .out_ready(m_axis_tready)
  );

endmodule
