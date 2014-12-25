
`timescale 1 ns / 1 ps

module axis_red_pitaya_adc_v1_0 #
(
  parameter integer ADC_DATA_WIDTH = 14,
  parameter integer AXIS_TDATA_WIDTH = 32
)
(
  // System signals
  output wire                        adc_clk,

  // ADC signals
  input  wire                        adc_clk_p,
  input  wire                        adc_clk_n,
  output wire                        adc_csn,
  input  wire [ADC_DATA_WIDTH-1:0]   adc_data_a,
  input  wire [ADC_DATA_WIDTH-1:0]   adc_data_b,

  // Master side
  output wire                        m_axis_tvalid,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata
);
  localparam ZERO_WIDTH = AXIS_TDATA_WIDTH/2 - ADC_DATA_WIDTH;

  reg  [ADC_DATA_WIDTH-1:0] int_data_a;
  reg  [ADC_DATA_WIDTH-1:0] int_data_b;
  wire                      int_adc_clk;

  IBUFGDS adc_clk_inst (.I(adc_clk_p), .IB(adc_clk_n), .O(int_adc_clk));

  always @(posedge int_adc_clk)
  begin
    int_data_a <= adc_data_a;
    int_data_b <= adc_data_b;
  end

  assign adc_clk = int_adc_clk;

  assign adc_csn = 1'b1;

  assign m_axis_tvalid = 1'b1;

  assign m_axis_tdata = {
    {(ZERO_WIDTH){1'b0}}, ~int_data_b[ADC_DATA_WIDTH-1], int_data_b[ADC_DATA_WIDTH-2:0],
    {(ZERO_WIDTH){1'b0}}, ~int_data_a[ADC_DATA_WIDTH-1], int_data_a[ADC_DATA_WIDTH-2:0]};

endmodule
