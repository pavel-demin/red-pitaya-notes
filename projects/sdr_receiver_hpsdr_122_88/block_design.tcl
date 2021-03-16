# Create clk_wiz
cell xilinx.com:ip:clk_wiz pll_0 {
  PRIMITIVE PLL
  PRIM_IN_FREQ.VALUE_SRC USER
  PRIM_IN_FREQ 122.88
  PRIM_SOURCE Differential_clock_capable_pin
  CLKOUT1_USED true
  CLKOUT1_REQUESTED_OUT_FREQ 122.88
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

# Create xlconstant
cell xilinx.com:ip:xlconstant const_0

# Create proc_sys_reset
cell xilinx.com:ip:proc_sys_reset rst_0 {} {
  ext_reset_in const_0/dout
}

# ADC

# Create axis_red_pitaya_adc
cell pavel-demin:user:axis_red_pitaya_adc adc_0 {
  ADC_DATA_WIDTH 16
} {
  aclk pll_0/clk_out1
  adc_dat_a adc_dat_a_i
  adc_dat_b adc_dat_b_i
  adc_csn adc_csn_o
}

# CFG

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register cfg_0 {
  CFG_DATA_WIDTH 640
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

# RX 0

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_0 {
  DIN_WIDTH 640 DIN_FROM 7 DIN_TO 0
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_0 {
  DIN_WIDTH 640 DIN_FROM 319 DIN_TO 32
} {
  din cfg_0/cfg_data
}

module rx_0 {
  source projects/sdr_receiver_hpsdr_122_88/rx.tcl
} {
  slice_0/din rst_slice_0/dout
  slice_1/din cfg_slice_0/dout
  slice_2/din cfg_slice_0/dout
  slice_3/din cfg_slice_0/dout
  slice_4/din cfg_slice_0/dout
  slice_5/din cfg_slice_0/dout
  slice_6/din cfg_slice_0/dout
  slice_7/din cfg_slice_0/dout
  slice_8/din cfg_slice_0/dout
  slice_9/din cfg_slice_0/dout
  slice_10/din cfg_slice_0/dout
  slice_11/din cfg_slice_0/dout
  slice_12/din cfg_slice_0/dout
  slice_13/din cfg_slice_0/dout
  slice_14/din cfg_slice_0/dout
  slice_15/din cfg_slice_0/dout
  slice_16/din cfg_slice_0/dout
  slice_17/din cfg_slice_0/dout
}

# RX 1

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_1 {
  DIN_WIDTH 640 DIN_FROM 327 DIN_TO 320
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_1 {
  DIN_WIDTH 640 DIN_FROM 639 DIN_TO 352
} {
  din cfg_0/cfg_data
}

module rx_1 {
  source projects/sdr_receiver_hpsdr_122_88/rx.tcl
} {
  slice_0/din rst_slice_1/dout
  slice_1/din cfg_slice_1/dout
  slice_2/din cfg_slice_1/dout
  slice_3/din cfg_slice_1/dout
  slice_4/din cfg_slice_1/dout
  slice_5/din cfg_slice_1/dout
  slice_6/din cfg_slice_1/dout
  slice_7/din cfg_slice_1/dout
  slice_8/din cfg_slice_1/dout
  slice_9/din cfg_slice_1/dout
  slice_10/din cfg_slice_1/dout
  slice_11/din cfg_slice_1/dout
  slice_12/din cfg_slice_1/dout
  slice_13/din cfg_slice_1/dout
  slice_14/din cfg_slice_1/dout
  slice_15/din cfg_slice_1/dout
  slice_16/din cfg_slice_1/dout
  slice_17/din cfg_slice_1/dout
}

# STS

# Create dna_reader
cell pavel-demin:user:dna_reader dna_0 {} {
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  NUM_PORTS 18
  IN0_WIDTH 32
  IN1_WIDTH 64
  IN2_WIDTH 16
  IN3_WIDTH 16
  IN4_WIDTH 16
  IN5_WIDTH 16
  IN6_WIDTH 16
  IN7_WIDTH 16
  IN8_WIDTH 16
  IN9_WIDTH 16
  IN10_WIDTH 16
  IN11_WIDTH 16
  IN12_WIDTH 16
  IN13_WIDTH 16
  IN14_WIDTH 16
  IN15_WIDTH 16
  IN16_WIDTH 16
  IN17_WIDTH 16
} {
  In0 const_0/dout
  In1 dna_0/dna_data
  In2 rx_0/fifo_generator_0/rd_data_count
  In3 rx_0/fifo_generator_1/rd_data_count
  In4 rx_0/fifo_generator_2/rd_data_count
  In5 rx_0/fifo_generator_3/rd_data_count
  In6 rx_0/fifo_generator_4/rd_data_count
  In7 rx_0/fifo_generator_5/rd_data_count
  In8 rx_0/fifo_generator_6/rd_data_count
  In9 rx_0/fifo_generator_7/rd_data_count
  In10 rx_1/fifo_generator_0/rd_data_count
  In11 rx_1/fifo_generator_1/rd_data_count
  In12 rx_1/fifo_generator_2/rd_data_count
  In13 rx_1/fifo_generator_3/rd_data_count
  In14 rx_1/fifo_generator_4/rd_data_count
  In15 rx_1/fifo_generator_5/rd_data_count
  In16 rx_1/fifo_generator_6/rd_data_count
  In17 rx_1/fifo_generator_7/rd_data_count
}

# Create axi_sts_register
cell pavel-demin:user:axi_sts_register sts_0 {
  STS_DATA_WIDTH 352
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
} {
  sts_data concat_0/dout
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins sts_0/S_AXI]

addr 0x40000000 4K sts_0/S_AXI

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins cfg_0/S_AXI]

addr 0x40001000 4K cfg_0/S_AXI

for {set i 0} {$i <= 7} {incr i} {

  # Create all required interconnections
  apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
    Master /ps_0/M_AXI_GP0
    Clk Auto
  } [get_bd_intf_pins rx_0/reader_$i/S_AXI]

  addr 0x4001[format %X [expr 2 * $i]]000 8K rx_0/reader_$i/S_AXI

  # Create all required interconnections
  apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
    Master /ps_0/M_AXI_GP0
    Clk Auto
  } [get_bd_intf_pins rx_1/reader_$i/S_AXI]

  addr 0x4002[format %X [expr 2 * $i]]000 8K rx_1/reader_$i/S_AXI

}
