# Create clk_wiz
cell xilinx.com:ip:clk_wiz:5.3 pll_0 {
  PRIMITIVE PLL
  PRIM_IN_FREQ.VALUE_SRC USER
  PRIM_IN_FREQ 125.0
  PRIM_SOURCE Differential_clock_capable_pin
  CLKOUT1_USED true
  CLKOUT1_REQUESTED_OUT_FREQ 125.0
  CLKOUT2_USED true
  CLKOUT2_REQUESTED_OUT_FREQ 250.0
  CLKOUT2_REQUESTED_PHASE -90.0
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

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register:1.0 cfg_0 {
  CFG_DATA_WIDTH 96
  AXI_ADDR_WIDTH 4
  AXI_DATA_WIDTH 32
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_0 {
  DIN_WIDTH 96 DIN_FROM 0 DIN_TO 0 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_1 {
  DIN_WIDTH 96 DIN_FROM 47 DIN_TO 32 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_2 {
  DIN_WIDTH 96 DIN_FROM 63 DIN_TO 48 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_3 {
  DIN_WIDTH 96 DIN_FROM 79 DIN_TO 64 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_4 {
  DIN_WIDTH 96 DIN_FROM 95 DIN_TO 80 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen:8.3 bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 16384
  WRITE_WIDTH_B 16
  ENABLE_A Always_Enabled
  ENABLE_B Always_Enabled
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
}

# Create axi_bram_writer
cell pavel-demin:user:axi_bram_writer:1.0 writer_0 {
  AXI_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 14
} {
  BRAM_PORTA bram_0/BRAM_PORTA
}

# Create axis_bram_reader
cell pavel-demin:user:axis_bram_reader:1.0 reader_0 {
  AXIS_TDATA_WIDTH 16
  BRAM_DATA_WIDTH 16
  BRAM_ADDR_WIDTH 15
  CONTINUOUS FALSE
} {
  BRAM_PORTA bram_0/BRAM_PORTB
  cfg_data slice_1/Dout
  aclk pll_0/clk_out1
  aresetn slice_0/Dout
}

# Create axis_zeroer
cell pavel-demin:user:axis_zeroer:1.0 zeroer_0 {
  AXIS_TDATA_WIDTH 16
} {
  S_AXIS reader_0/M_AXIS
  aclk pll_0/clk_out1
}

# Create xbip_dsp48_macro
cell xilinx.com:ip:xbip_dsp48_macro:3.0 dsp_0 {
  INSTRUCTION1 A*B
  A_WIDTH.VALUE_SRC USER
  B_WIDTH.VALUE_SRC USER
  OUTPUT_PROPERTIES User_Defined
  A_WIDTH 16
  B_WIDTH 16
  P_WIDTH 32
} {
  B slice_2/Dout
  CLK pll_0/clk_out1
}

# Create xbip_dsp48_macro
cell xilinx.com:ip:xbip_dsp48_macro:3.0 dsp_1 {
  INSTRUCTION1 A*B+C
  PIPELINE_OPTIONS Expert
  AREG_3 false
  AREG_4 false
  CREG_3 false
  CREG_4 false
  MREG_5 false
  PREG_6 false
  A_WIDTH.VALUE_SRC USER
  B_WIDTH.VALUE_SRC USER
  C_WIDTH.VALUE_SRC USER
  OUTPUT_PROPERTIES User_Defined
  A_WIDTH 25
  B_WIDTH 16
  C_WIDTH 41
  P_WIDTH 41
} {
  B slice_3/Dout
  CLK pll_0/clk_out1
}

cell xilinx.com:ip:xbip_dsp48_macro:3.0 dsp_2 {
  INSTRUCTION1 A*B+C
  PIPELINE_OPTIONS Expert
  AREG_3 false
  AREG_4 false
  CREG_3 false
  CREG_4 false
  MREG_5 false
  PREG_6 false
  A_WIDTH.VALUE_SRC USER
  B_WIDTH.VALUE_SRC USER
  C_WIDTH.VALUE_SRC USER
  OUTPUT_PROPERTIES User_Defined
  A_WIDTH 25
  B_WIDTH 16
  C_WIDTH 41
  P_WIDTH 41
} {
  B slice_4/Dout
  CLK pll_0/clk_out1
}

# Create axis_iir_filter
cell pavel-demin:user:axis_iir_filter:1.0 iir_0 {
  AXIS_TDATA_WIDTH 16
} {
  S_AXIS zeroer_0/M_AXIS
  dsp_a_a dsp_0/A
  dsp_a_p dsp_0/P
  dsp_b_a dsp_1/A
  dsp_b_c dsp_1/C
  dsp_b_p dsp_1/P
  dsp_c_a dsp_2/A
  dsp_c_c dsp_2/C
  dsp_c_p dsp_2/P
  aclk pll_0/clk_out1
  aresetn slice_0/Dout
}

# Create axis_data_fifo
cell xilinx.com:ip:axis_data_fifo:1.1 fifo_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
  FIFO_DEPTH 16384
} {
  S_AXIS iir_0/M_AXIS
  s_axis_tvalid const_0/dout
  s_axis_aclk pll_0/clk_out1
  s_axis_aresetn slice_0/Dout
}

# Create axi_axis_reader
cell pavel-demin:user:axi_axis_reader:1.0 reader_1 {
  AXI_DATA_WIDTH 32
} {
  S_AXIS fifo_0/M_AXIS
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins cfg_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_cfg_0_reg0]
set_property OFFSET 0x40001000 [get_bd_addr_segs ps_0/Data/SEG_cfg_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins reader_1/S_AXI]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_reader_1_reg0]
set_property OFFSET 0x40010000 [get_bd_addr_segs ps_0/Data/SEG_reader_1_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins writer_0/S_AXI]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]
set_property OFFSET 0x40030000 [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]

