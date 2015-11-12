# Create processing_system7
cell xilinx.com:ip:processing_system7:5.5 ps_0 {
  PCW_IMPORT_BOARD_PRESET cfg/red_pitaya.xml
  PCW_USE_S_AXI_HP0 1
} {
  M_AXI_GP0_ACLK ps_0/FCLK_CLK0
  S_AXI_HP0_ACLK ps_0/FCLK_CLK0
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {
  make_external {FIXED_IO, DDR}
  Master Disable
  Slave Disable
} [get_bd_cells ps_0]

# Create proc_sys_reset
cell xilinx.com:ip:proc_sys_reset:5.0 rst_0

# Create util_ds_buf
cell xilinx.com:ip:util_ds_buf:2.1 buf_0 {
  C_SIZE 2
  C_BUF_TYPE IBUFDS
} {
  IBUF_DS_P daisy_p_i
  IBUF_DS_N daisy_n_i
}

# Create util_ds_buf
cell xilinx.com:ip:util_ds_buf:2.1 buf_1 {
  C_SIZE 2
  C_BUF_TYPE OBUFDS
} {
  OBUF_DS_P daisy_p_o
  OBUF_DS_N daisy_n_o
}

# Create axis_red_pitaya_adc
cell pavel-demin:user:axis_red_pitaya_adc:1.0 adc_0 {} {
  adc_clk_p adc_clk_p_i
  adc_clk_n adc_clk_n_i
  adc_dat_a adc_dat_a_i
  adc_dat_b adc_dat_b_i
  adc_csn adc_csn_o
}

# Create input/output port
create_bd_port -dir IO -from 0 -to 0 exp_p_trg

# Create axis_gpio_reader
cell pavel-demin:user:axis_gpio_reader:1.0 gpio_0 {
  AXIS_TDATA_WIDTH 1
} {
  gpio_data exp_p_trg
  aclk adc_0/adc_clk
}

# Create c_counter_binary
cell xilinx.com:ip:c_counter_binary:12.0 cntr_0 {
  Output_Width 32
} {
  CLK adc_0/adc_clk
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_0 {
  DIN_WIDTH 32 DIN_FROM 26 DIN_TO 26 DOUT_WIDTH 1
} {
  Din cntr_0/Q
}

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register:1.0 cfg_0 {
  CFG_DATA_WIDTH 128
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins cfg_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_cfg_0_reg0]
set_property OFFSET 0x40000000 [get_bd_addr_segs ps_0/Data/SEG_cfg_0_reg0]

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_1 {
  DIN_WIDTH 128 DIN_FROM 0 DIN_TO 0 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_2 {
  DIN_WIDTH 128 DIN_FROM 1 DIN_TO 1 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_3 {
  DIN_WIDTH 128 DIN_FROM 16 DIN_TO 16 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_4 {
  DIN_WIDTH 128 DIN_FROM 47 DIN_TO 32 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_5 {
  DIN_WIDTH 128 DIN_FROM 63 DIN_TO 48 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_6 {
  DIN_WIDTH 128 DIN_FROM 79 DIN_TO 64 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_7 {
  DIN_WIDTH 128 DIN_FROM 95 DIN_TO 80 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_8 {
  DIN_WIDTH 128 DIN_FROM 103 DIN_TO 96 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_0 {
  IN1_WIDTH 7
} {
  In0 slice_0/Dout
  In1 slice_8/Dout
  dout led_o
}

# Delete input/output port
delete_bd_objs [get_bd_ports exp_n_tri_io]

# Create input/output port
create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 exp_n

# Create axi_gpio
cell xilinx.com:ip:axi_gpio:2.0 gpio_1 {
  C_GPIO_WIDTH 8
} {
  GPIO exp_n
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins gpio_1/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_gpio_1_Reg]
set_property OFFSET 0x40002000 [get_bd_addr_segs ps_0/Data/SEG_gpio_1_Reg]

# Delete input/output port
delete_bd_objs [get_bd_ports exp_p_tri_io]

# Create input/output port
create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 exp_p

# Create axi_gpio
cell xilinx.com:ip:axi_gpio:2.0 gpio_2 {
  C_GPIO_WIDTH 7
} {
  GPIO exp_p
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins gpio_2/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_gpio_2_Reg]
set_property OFFSET 0x40003000 [get_bd_addr_segs ps_0/Data/SEG_gpio_2_Reg]

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner:1.1 comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
} {
  S00_AXIS adc_0/M_AXIS
  S01_AXIS gpio_0/M_AXIS
  aclk adc_0/adc_clk
  aresetn const_0/dout
}

# Create axis_clock_converter
cell xilinx.com:ip:axis_clock_converter:1.1 fifo_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 5
} {
  S_AXIS comb_0/M_AXIS
  s_axis_aclk adc_0/adc_clk
  s_axis_aresetn const_0/dout
  m_axis_aclk ps_0/FCLK_CLK0
  m_axis_aresetn rst_0/peripheral_aresetn
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster:1.1 bcast_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 8
  M_TDATA_NUM_BYTES 4
  NUM_MI 2
  M00_TDATA_REMAP {tdata[31:0]}
  M01_TDATA_REMAP {tdata[63:32]}
} {
  S_AXIS fifo_0/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_decimator
cell pavel-demin:user:axis_decimator:1.0 dcmtr_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 16
} {
  S_AXIS bcast_0/M00_AXIS
  cfg_data slice_7/Dout
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter:1.1 subset_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 1
  TDATA_REMAP {tdata[7:0]}
} {
  S_AXIS bcast_0/M01_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_trigger
cell pavel-demin:user:axis_trigger:1.0 trig_0 {
  AXIS_TDATA_WIDTH 8
  AXIS_TDATA_SIGNED FALSE
} {
  S_AXIS subset_0/M_AXIS
  pol_data slice_3/Dout
  msk_data slice_4/Dout
  lvl_data slice_5/Dout
  aclk ps_0/FCLK_CLK0
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_1

# Create axis_packetizer
cell pavel-demin:user:axis_oscilloscope:1.0 scope_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 14
} {
  S_AXIS dcmtr_0/M_AXIS
  run_flag trig_0/trg_flag
  trg_flag const_1/dout
  pre_data const_1/dout
  tot_data slice_6/Dout
  aclk ps_0/FCLK_CLK0
  aresetn slice_1/Dout
}

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen:8.3 bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  USE_BYTE_WRITE_ENABLE true
  BYTE_SIZE 8
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 16384
  WRITE_WIDTH_B 32
  WRITE_DEPTH_B 16384
  ENABLE_A Always_Enabled
  ENABLE_B Always_Enabled
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
}

# Create axis_bram_writer
cell pavel-demin:user:axis_bram_writer:1.0 writer_0 {
  AXIS_TDATA_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 14
} {
  S_AXIS scope_0/M_AXIS
  BRAM_PORTA bram_0/BRAM_PORTA
  aclk ps_0/FCLK_CLK0
  aresetn slice_2/Dout
}

# Create axi_bram_reader
cell pavel-demin:user:axi_bram_reader:1.0 reader_0 {
  AXI_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 14
} {
  BRAM_PORTA bram_0/BRAM_PORTB
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins reader_0/S_AXI]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]
set_property OFFSET 0x40010000 [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_1 {
  NUM_PORTS 2
  IN0_WIDTH 16
  IN1_WIDTH 16
} {
  In0 scope_0/sts_data
  In1 writer_0/sts_data
}

# Create axi_sts_register
cell pavel-demin:user:axi_sts_register:1.0 sts_0 {
  STS_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
} {
  sts_data concat_1/dout
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins sts_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_sts_0_reg0]
set_property OFFSET 0x40001000 [get_bd_addr_segs ps_0/Data/SEG_sts_0_reg0]
