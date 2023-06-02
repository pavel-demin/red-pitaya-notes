
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
  reg [AXIS_TDATA_WIDTH-1:0] int_data_reg[1:0], int_data_next[1:0];
  reg [AXIS_TDATA_WIDTH-1:0] int_min_reg, int_min_next;
  reg [AXIS_TDATA_WIDTH-1:0] int_tdata_reg, int_tdata_next;
  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg int_enbl_reg, int_enbl_next;
  reg int_rising_reg, int_rising_next;
  reg int_tvalid_reg, int_tvalid_next;

  wire [AXIS_TDATA_WIDTH-1:0] int_tdata_wire;
  wire int_mincut_wire, int_maxcut_wire, int_rising_wire, int_delay_wire;

  assign int_delay_wire = int_cntr_reg < cfg_data;

  generate
    if(AXIS_TDATA_SIGNED == "TRUE")
    begin : SIGNED
      assign int_rising_wire = $signed(int_data_reg[1]) < $signed(s_axis_tdata);
      assign int_tdata_wire = $signed(int_data_reg[0]) - $signed(int_min_reg);
      assign int_mincut_wire = $signed(int_tdata_wire) > $signed(min_data);
      assign int_maxcut_wire = $signed(int_tdata_wire) < $signed(max_data);
    end
    else
    begin : UNSIGNED
      assign int_rising_wire = int_data_reg[1] < s_axis_tdata;
      assign int_tdata_wire = int_data_reg[0] - int_min_reg;
      assign int_mincut_wire = int_tdata_wire > min_data;
      assign int_maxcut_wire = int_tdata_wire < max_data;
    end
  endgenerate

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_data_reg[0] <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_data_reg[1] <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_tdata_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_min_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
      int_rising_reg <= 1'b0;
      int_tvalid_reg <= 1'b0;
    end
    else
    begin
      int_data_reg[0] <= int_data_next[0];
      int_data_reg[1] <= int_data_next[1];
      int_tdata_reg <= int_tdata_next;
      int_min_reg <= int_min_next;
      int_cntr_reg <= int_cntr_next;
      int_enbl_reg <= int_enbl_next;
      int_rising_reg <= int_rising_next;
      int_tvalid_reg <= int_tvalid_next;
    end
  end

  always @*
  begin
    int_data_next[0] = int_data_reg[0];
    int_data_next[1] = int_data_reg[1];
    int_tdata_next = int_tdata_reg;
    int_min_next = int_min_reg;
    int_cntr_next = int_cntr_reg;
    int_enbl_next = int_enbl_reg;
    int_rising_next = int_rising_reg;
    int_tvalid_next = int_tvalid_reg;

    if(s_axis_tvalid)
    begin
      int_data_next[0] = s_axis_tdata;
      int_data_next[1] = int_data_reg[0];
      int_rising_next = int_rising_wire;
    end

    if(s_axis_tvalid & int_delay_wire)
    begin
      int_cntr_next = int_cntr_reg + 1'b1;
    end

    // minimum after delay
    if(s_axis_tvalid & ~int_delay_wire & ~int_rising_reg & int_rising_wire)
    begin
      int_min_next = int_data_reg[1];
      int_enbl_next = 1'b1;
    end

    // maximum after minimum
    if(s_axis_tvalid & int_enbl_reg & int_rising_reg & ~int_rising_wire & int_mincut_wire)
    begin
      int_tdata_next = int_tdata_wire;
      int_tvalid_next = int_maxcut_wire;
      int_cntr_next = {(CNTR_WIDTH){1'b0}};
      int_enbl_next = 1'b0;
    end

    if(m_axis_tready & int_tvalid_reg)
    begin
      int_tvalid_next = 1'b0;
    end
  end

  assign s_axis_tready = 1'b1;
  assign m_axis_tdata = int_tdata_reg;
  assign m_axis_tvalid = int_tvalid_reg;

endmodule
