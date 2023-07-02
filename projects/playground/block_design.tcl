# Create clk_wiz
cell xilinx.com:ip:clk_wiz pll_0 {
  PRIMITIVE PLL
  PRIM_IN_FREQ.VALUE_SRC USER
  PRIM_IN_FREQ 125.0
  PRIM_SOURCE Differential_clock_capable_pin
  CLKOUT1_USED true
  CLKOUT1_REQUESTED_OUT_FREQ 125.0
  CLKOUT2_USED true
  CLKOUT2_REQUESTED_OUT_FREQ 250.0
  CLKOUT2_REQUESTED_PHASE 157.5
  CLKOUT3_USED true
  CLKOUT3_REQUESTED_OUT_FREQ 250.0
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

# Create xlconstant
cell xilinx.com:ip:xlconstant const_0

# Create proc_sys_reset
cell xilinx.com:ip:proc_sys_reset rst_0 {} {
  ext_reset_in const_0/dout
  dcm_locked pll_0/locked
  slowest_sync_clk pll_0/clk_out1
}

# HUB

# Create axi_hub
cell pavel-demin:user:axi_hub hub_0 {
  CFG_DATA_WIDTH 96
  STS_DATA_WIDTH 64
} {
  S_AXI ps_0/M_AXI_GP0
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 96 DIN_FROM 0 DIN_TO 0
} {
  din hub_0/cfg_data
}

# LED

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 96 DIN_FROM 39 DIN_TO 32
} {
  din hub_0/cfg_data
  dout led_o
}

# DSP48

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 96 DIN_FROM 79 DIN_TO 64
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 96 DIN_FROM 95 DIN_TO 80
} {
  din hub_0/cfg_data
}


# Create dsp_macro
cell xilinx.com:ip:dsp_macro dsp_0 {
  INSTRUCTION1 A*B
  A_WIDTH.VALUE_SRC USER
  B_WIDTH.VALUE_SRC USER
  OUTPUT_PROPERTIES User_Defined
  A_WIDTH 16
  B_WIDTH 16
  P_WIDTH 32
} {
  A slice_2/dout
  B slice_3/dout
  CLK pll_0/clk_out1
}

# COUNTER

# Create xlconstant
cell xilinx.com:ip:xlconstant const_1 {
  CONST_WIDTH 32
  CONST_VAL 4294967295
}

# Create axis_counter
cell pavel-demin:user:axis_counter cntr_0 {
  CONTINUOUS TRUE
} {
  M_AXIS hub_0/S01_AXIS
  cfg_data const_1/dout
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# XADC

# Create xadc_bram
cell pavel-demin:user:xadc_bram xadc_0 {} {
  B_BRAM hub_0/B02_BRAM
  Vp_Vn Vp_Vn
  Vaux0 Vaux0
  Vaux1 Vaux1
  Vaux8 Vaux8
  Vaux9 Vaux9
}

# ADC

# Create axis_red_pitaya_adc
cell pavel-demin:user:axis_red_pitaya_adc adc_0 {
  ADC_DATA_WIDTH 14
} {
  aclk pll_0/clk_out1
  adc_dat_a adc_dat_a_i
  adc_dat_b adc_dat_b_i
  adc_csn adc_csn_o
}

# FIFO

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_0 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
  WRITE_DEPTH 16384
} {
  S_AXIS adc_0/M_AXIS
  M_AXIS hub_0/S00_AXIS
  aclk pll_0/clk_out1
  aresetn slice_0/dout
}

# BRAM

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  USE_BYTE_WRITE_ENABLE true
  BYTE_SIZE 8
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 16384
  WRITE_WIDTH_B 32
  REGISTER_PORTA_OUTPUT_OF_MEMORY_PRIMITIVES false
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
} {
  BRAM_PORTA hub_0/B00_BRAM
}

# Create xlconstant
cell xilinx.com:ip:xlconstant const_2 {
  CONST_WIDTH 14
  CONST_VAL 16383
}

# Create axis_bram_reader
cell pavel-demin:user:axis_bram_reader reader_0 {
  AXIS_TDATA_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 14
  CONTINUOUS TRUE
} {
  B_BRAM bram_0/BRAM_PORTB
  cfg_data const_2/dout
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# DAC

# Create axis_red_pitaya_dac
cell pavel-demin:user:axis_red_pitaya_dac dac_0 {
  DAC_DATA_WIDTH 14
} {
  aclk pll_0/clk_out1
  ddr_clk pll_0/clk_out2
  wrt_clk pll_0/clk_out3
  locked pll_0/locked
  S_AXIS reader_0/M_AXIS
  dac_clk dac_clk_o
  dac_rst dac_rst_o
  dac_sel dac_sel_o
  dac_wrt dac_wrt_o
  dac_dat dac_dat_o
}

# STS

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  NUM_PORTS 2
  IN0_WIDTH 32
  IN1_WIDTH 32
} {
  In0 fifo_0/read_count
  In1 dsp_0/P
  dout hub_0/sts_data
}
