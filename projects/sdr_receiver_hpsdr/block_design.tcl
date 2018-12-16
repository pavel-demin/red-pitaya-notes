# Create clk_wiz
cell xilinx.com:ip:clk_wiz:6.0 pll_0 {
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
cell xilinx.com:ip:processing_system7:5.5 ps_0 {
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
cell xilinx.com:ip:xlconstant:1.1 const_0

# Create proc_sys_reset
cell xilinx.com:ip:proc_sys_reset:5.0 rst_0 {} {
  ext_reset_in const_0/dout
}

# ADC

# Create axis_red_pitaya_adc
cell pavel-demin:user:axis_red_pitaya_adc:2.0 adc_0 {
  ADC_DATA_WIDTH 16
} {
  aclk pll_0/clk_out1
  adc_dat_a adc_dat_a_i
  adc_dat_b adc_dat_b_i
  adc_csn adc_csn_o
}

# CFG

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register:1.0 cfg_0 {
  CFG_DATA_WIDTH 320
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

# RX 0

# Create port_slicer
cell pavel-demin:user:port_slicer:1.0 rst_slice_0 {
  DIN_WIDTH 320 DIN_FROM 7 DIN_TO 0
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer:1.0 cfg_slice_0 {
  DIN_WIDTH 320 DIN_FROM 319 DIN_TO 32
} {
  din cfg_0/cfg_data
}

module rx_0 {
  source projects/sdr_receiver_hpsdr/rx.tcl
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

# STS

# Create dna_reader
cell pavel-demin:user:dna_reader:1.0 dna_0 {} {
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_0 {
  NUM_PORTS 10
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
}

# Create axi_sts_register
cell pavel-demin:user:axi_sts_register:1.0 sts_0 {
  STS_DATA_WIDTH 224
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

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_sts_0_reg0]
set_property OFFSET 0x40000000 [get_bd_addr_segs ps_0/Data/SEG_sts_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins cfg_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_cfg_0_reg0]
set_property OFFSET 0x40001000 [get_bd_addr_segs ps_0/Data/SEG_cfg_0_reg0]

for {set i 0} {$i <= 7} {incr i} {

  # Create all required interconnections
  apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
    Master /ps_0/M_AXI_GP0
    Clk Auto
  } [get_bd_intf_pins rx_0/reader_$i/S_AXI]

  set_property RANGE 8K [get_bd_addr_segs ps_0/Data/SEG_reader_${i}_reg0]
  set_property OFFSET 0x4001[format %X [expr 2 * $i]]000 [get_bd_addr_segs ps_0/Data/SEG_reader_${i}_reg0]

}
