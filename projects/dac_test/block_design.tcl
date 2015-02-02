source projects/packetizer_test/block_design.tcl

# Create clk_wiz
cell xilinx.com:ip:clk_wiz:5.1 pll_0 {
  PRIMITIVE PLL
  PRIM_IN_FREQ.VALUE_SRC USER
  PRIM_IN_FREQ 125.0
  CLKOUT1_USED true
  CLKOUT2_USED true
  CLKOUT1_REQUESTED_OUT_FREQ 125.0
  CLKOUT2_REQUESTED_OUT_FREQ 250.0
} {
  clk_in1 adc_0/adc_clk
}

# Create dds_compiler
cell xilinx.com:ip:dds_compiler:6.0 dds_0 {
  DDS_CLOCK_RATE 125
  SPURIOUS_FREE_DYNAMIC_RANGE 83
  FREQUENCY_RESOLUTION 0.5
  OUTPUT_SELECTION Sine
  HAS_PHASE_OUT false
  OUTPUT_FREQUENCY1 0.9765625
} {
  aclk pll_0/clk_out1
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_3

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster:1.1 bcast_0 {} {
  aclk pll_0/clk_out1
  aresetn const_3/dout
  S_AXIS dds_0/M_AXIS_DATA
}

# Create axis_combiner
cell xilinx.com:ip:axis_combiner:1.1 comb_0 {} {
  aclk pll_0/clk_out1
  aresetn const_3/dout
  S00_AXIS bcast_0/M00_AXIS
  S01_AXIS bcast_0/M01_AXIS
}

# Create axis_red_pitaya_dac
cell pavel-demin:user:axis_red_pitaya_dac:1.0 dac_0 {} {
  aclk pll_0/clk_out1
  ddr_clk pll_0/clk_out2
  locked pll_0/locked
  S_AXIS comb_0/M_AXIS
  dac_clk dac_clk_o
  dac_rst dac_rst_o
  dac_sel dac_sel_o
  dac_wrt dac_wrt_o
  dac_dat dac_dat_o
}
