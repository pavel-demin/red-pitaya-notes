# Create clk_wiz
cell xilinx.com:ip:clk_wiz pll_0 {
  PRIMITIVE PLL
  PRIM_IN_FREQ.VALUE_SRC USER
  PRIM_IN_FREQ 125.0
  PRIM_SOURCE Differential_clock_capable_pin
  CLKOUT1_USED true
  CLKOUT1_REQUESTED_OUT_FREQ 125.0
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
  ADC_DATA_WIDTH 14
} {
  aclk pll_0/clk_out1
  adc_dat_a adc_dat_a_i
  adc_dat_b adc_dat_b_i
  adc_csn adc_csn_o
}

# Create input/output port
create_bd_port -dir IO -from 0 -to 0 exp_p_trg

# Create axis_gpio_reader
cell pavel-demin:user:axis_gpio_reader gpio_0 {
  AXIS_TDATA_WIDTH 1
} {
  gpio_data exp_p_trg
  aclk pll_0/clk_out1
}

# Create c_counter_binary
cell xilinx.com:ip:c_counter_binary cntr_0 {
  Output_Width 32
} {
  CLK pll_0/clk_out1
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 32 DIN_FROM 26 DIN_TO 26
} {
  din cntr_0/Q
}

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register cfg_0 {
  CFG_DATA_WIDTH 128
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

addr 0x40000000 4K cfg_0/S_AXI /ps_0/M_AXI_GP0

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 128 DIN_FROM 0 DIN_TO 0
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 128 DIN_FROM 1 DIN_TO 1
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 128 DIN_FROM 16 DIN_TO 16
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_4 {
  DIN_WIDTH 128 DIN_FROM 47 DIN_TO 32
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_5 {
  DIN_WIDTH 128 DIN_FROM 63 DIN_TO 48
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_6 {
  DIN_WIDTH 128 DIN_FROM 79 DIN_TO 64
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_7 {
  DIN_WIDTH 128 DIN_FROM 95 DIN_TO 80
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_8 {
  DIN_WIDTH 128 DIN_FROM 103 DIN_TO 96
} {
  din cfg_0/cfg_data
}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  IN1_WIDTH 7
} {
  In0 slice_0/dout
  In1 slice_8/dout
  dout led_o
}

# Delete input/output port
delete_bd_objs [get_bd_ports exp_n_tri_io]

# Create input/output port
create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 exp_n

# Create axi_gpio
cell xilinx.com:ip:axi_gpio gpio_1 {
  C_GPIO_WIDTH 8
} {
  GPIO exp_n
}

addr 0x40002000 4K gpio_1/S_AXI /ps_0/M_AXI_GP0

# Delete input/output port
delete_bd_objs [get_bd_ports exp_p_tri_io]

# Create input/output port
create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 exp_p

# Create axi_gpio
cell xilinx.com:ip:axi_gpio gpio_2 {
  C_GPIO_WIDTH 7
} {
  GPIO exp_p
}

addr 0x40003000 4K gpio_2/S_AXI /ps_0/M_AXI_GP0

# Create axis_decimator
cell pavel-demin:user:axis_decimator dcmtr_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 16
} {
  S_AXIS adc_0/M_AXIS
  cfg_data slice_7/dout
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create axis_trigger
cell pavel-demin:user:axis_trigger trig_0 {
  AXIS_TDATA_WIDTH 8
  AXIS_TDATA_SIGNED FALSE
} {
  S_AXIS gpio_0/M_AXIS
  pol_data slice_3/dout
  msk_data slice_4/dout
  lvl_data slice_5/dout
  aclk pll_0/clk_out1
}

# Create axis_packetizer
cell pavel-demin:user:axis_oscilloscope scope_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 14
} {
  S_AXIS dcmtr_0/M_AXIS
  run_flag trig_0/trg_flag
  trg_flag const_0/dout
  pre_data const_0/dout
  tot_data slice_6/dout
  aclk pll_0/clk_out1
  aresetn slice_1/dout
}

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 16384
  WRITE_WIDTH_B 32
  ENABLE_A Always_Enabled
  ENABLE_B Always_Enabled
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
}

# Create axis_bram_writer
cell pavel-demin:user:axis_bram_writer writer_0 {
  AXIS_TDATA_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 14
} {
  S_AXIS scope_0/M_AXIS
  BRAM_PORTA bram_0/BRAM_PORTA
  aclk pll_0/clk_out1
  aresetn slice_2/dout
}

# Create axi_bram_reader
cell pavel-demin:user:axi_bram_reader reader_0 {
  AXI_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 14
} {
  BRAM_PORTA bram_0/BRAM_PORTB
}

addr 0x40010000 64K reader_0/S_AXI /ps_0/M_AXI_GP0

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_1 {
  NUM_PORTS 2
  IN0_WIDTH 16
  IN1_WIDTH 16
} {
  In0 scope_0/sts_data
  In1 writer_0/sts_data
}

# Create axi_sts_register
cell pavel-demin:user:axi_sts_register sts_0 {
  STS_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
} {
  sts_data concat_1/dout
}

addr 0x40001000 4K sts_0/S_AXI /ps_0/M_AXI_GP0
