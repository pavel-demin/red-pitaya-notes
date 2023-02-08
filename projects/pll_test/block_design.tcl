# Create clk_wiz
cell xilinx.com:ip:clk_wiz pll_0 {
  PRIMITIVE PLL
  PRIM_IN_FREQ.VALUE_SRC USER
  PRIM_IN_FREQ 125.0
  PRIM_SOURCE Differential_clock_capable_pin
  CLKOUT1_USED true
  CLKOUT1_REQUESTED_OUT_FREQ 12.50
  CLKOUT2_USED true
  CLKOUT2_REQUESTED_OUT_FREQ 25.00
  CLKOUT2_REQUESTED_PHASE 157.5
  CLKOUT3_USED true
  CLKOUT3_REQUESTED_OUT_FREQ 25.00
  CLKOUT3_REQUESTED_PHASE 202.5
  USE_RESET false
} {
  clk_in1_p adc_clk_p_i
  clk_in1_n adc_clk_n_i
}

# Create processing_system7
cell xilinx.com:ip:processing_system7 ps_0 {
  PCW_IMPORT_BOARD_PRESET cfg/red_pitaya.xml
} {
  M_AXI_GP0_ACLK pll_0/clk_out1
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {
  make_external {FIXED_IO, DDR}
  Master Disable
  Slave Disable
} [get_bd_cells ps_0]

# Create axis_red_pitaya_dac
cell pavel-demin:user:axis_red_pitaya_dac dac_0 {
  DAC_DATA_WIDTH 14
} {
  aclk pll_0/clk_out1
  ddr_clk pll_0/clk_out2
  wrt_clk pll_0/clk_out3
  locked pll_0/locked
}

# Delete input/output port
delete_bd_objs [get_bd_ports exp_n_tri_io]

# Create output port
create_bd_port -dir O -from 7 -to 0 exp_n_tri_io

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  NUM_PORTS 3
  IN0_WIDTH 1
  IN1_WIDTH 1
  IN2_WIDTH 1
} {
  In0 dac_0/dac_sel
  In1 dac_0/dac_clk
  In2 dac_0/dac_wrt
  dout exp_n_tri_io
}
