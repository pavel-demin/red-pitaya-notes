# Create processing_system7
cell xilinx.com:ip:processing_system7:5.5 ps_0 {
  PCW_IMPORT_BOARD_PRESET cfg/red_pitaya.xml
} {
  M_AXI_GP0_ACLK ps_0/FCLK_CLK0
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

# ADC

# Create axis_red_pitaya_adc
cell pavel-demin:user:axis_red_pitaya_adc:1.0 adc_0 {} {
  adc_clk_p adc_clk_p_i
  adc_clk_n adc_clk_n_i
  adc_dat_a adc_dat_a_i
  adc_dat_b adc_dat_b_i
  adc_csn adc_csn_o
}

# LED

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
  Dout led_o
}

# DAC

# Create clk_wiz
cell xilinx.com:ip:clk_wiz:5.2 pll_0 {
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

# Create axis_red_pitaya_dac
cell pavel-demin:user:axis_red_pitaya_dac:1.0 dac_0 {} {
  aclk pll_0/clk_out1
  ddr_clk pll_0/clk_out2
  locked pll_0/locked
  dac_clk dac_clk_o
  dac_rst dac_rst_o
  dac_sel dac_sel_o
  dac_wrt dac_wrt_o
  dac_dat dac_dat_o
}

# CFG

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register:1.0 cfg_0 {
  CFG_DATA_WIDTH 448
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_1 {
  DIN_WIDTH 448 DIN_FROM 0 DIN_TO 0 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_2 {
  DIN_WIDTH 448 DIN_FROM 1 DIN_TO 1 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_3 {
  DIN_WIDTH 448 DIN_FROM 2 DIN_TO 2 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_4 {
  DIN_WIDTH 448 DIN_FROM 3 DIN_TO 3 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_5 {
  DIN_WIDTH 448 DIN_FROM 31 DIN_TO 16 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_6 {
  DIN_WIDTH 448 DIN_FROM 63 DIN_TO 32 DOUT_WIDTH 32
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_7 {
  DIN_WIDTH 448 DIN_FROM 159 DIN_TO 64 DOUT_WIDTH 96
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_8 {
  DIN_WIDTH 448 DIN_FROM 255 DIN_TO 160 DOUT_WIDTH 96
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_9 {
  DIN_WIDTH 448 DIN_FROM 351 DIN_TO 256 DOUT_WIDTH 96
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_10 {
  DIN_WIDTH 448 DIN_FROM 447 DIN_TO 352 DOUT_WIDTH 96
} {
  Din cfg_0/cfg_data
}

# Create axis_packetizer
cell pavel-demin:user:pulse_generator:1.0 gen_0 {} {
  cfg slice_7/Dout
  aclk ps_0/FCLK_CLK0
  aresetn slice_3/Dout
}

# Create axis_packetizer
cell pavel-demin:user:pulse_generator:1.0 gen_1 {} {
  cfg slice_8/Dout
  aclk ps_0/FCLK_CLK0
  aresetn slice_3/Dout
}

# Create axis_packetizer
cell pavel-demin:user:pulse_generator:1.0 gen_2 {} {
  cfg slice_9/Dout
  aclk ps_0/FCLK_CLK0
  aresetn slice_3/Dout
}

# Create axis_packetizer
cell pavel-demin:user:pulse_generator:1.0 gen_3 {} {
  cfg slice_10/Dout
  aclk ps_0/FCLK_CLK0
  aresetn slice_3/Dout
}

# Delete input/output port
delete_bd_objs [get_bd_ports exp_n_tri_io]

# Create output port
create_bd_port -dir O -from 7 -to 0 exp_n_tri_io

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_0 {
  NUM_PORTS 2
  IN0_WIDTH 1
  IN1_WIDTH 1
} {
  In0 gen_2/out
  In1 gen_3/out
  dout exp_n_tri_io
}

# RX

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0

# Create axis_clock_converter
cell xilinx.com:ip:axis_clock_converter:1.1 fifo_0 {} {
  S_AXIS adc_0/M_AXIS
  s_axis_aclk adc_0/adc_clk
  s_axis_aresetn const_0/dout
  m_axis_aclk ps_0/FCLK_CLK0
  m_axis_aresetn rst_0/peripheral_aresetn
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster:1.1 bcast_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 2
  M00_TDATA_REMAP {tdata[15:0]}
  M01_TDATA_REMAP {tdata[31:16]}
} {
  S_AXIS fifo_0/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_1

# Create axis_oscilloscope
cell pavel-demin:user:axis_oscilloscope:1.0 scope_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 32
} {
  S_AXIS bcast_0/M00_AXIS
  run_flag gen_0/out
  trg_flag const_1/dout
  pre_data const_1/dout
  tot_data slice_6/Dout
  aclk ps_0/FCLK_CLK0
  aresetn slice_4/Dout
}

# Create axis_oscilloscope
cell pavel-demin:user:axis_oscilloscope:1.0 scope_1 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 32
} {
  S_AXIS bcast_0/M01_AXIS
  run_flag gen_0/out
  trg_flag const_1/dout
  pre_data const_1/dout
  tot_data slice_6/Dout
  aclk ps_0/FCLK_CLK0
  aresetn slice_4/Dout
}

# Create axis_variable
cell pavel-demin:user:axis_accumulator:1.0 accu_0 {
  S_AXIS_TDATA_WIDTH 16
  M_AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 16
  AXIS_TDATA_SIGNED TRUE
} {
  S_AXIS scope_0/M_AXIS
  cfg_data slice_5/Dout
  aclk ps_0/FCLK_CLK0
  aresetn slice_4/Dout
}

# Create axis_variable
cell pavel-demin:user:axis_accumulator:1.0 accu_1 {
  S_AXIS_TDATA_WIDTH 16
  M_AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 16
  AXIS_TDATA_SIGNED TRUE
} {
  S_AXIS scope_1/M_AXIS
  cfg_data slice_5/Dout
  aclk ps_0/FCLK_CLK0
  aresetn slice_4/Dout
}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner:1.1 comb_1 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
} {
  S00_AXIS accu_0/M_AXIS
  S01_AXIS accu_1/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create fifo_generator
cell xilinx.com:ip:fifo_generator:13.0 fifo_generator_0 {
  PERFORMANCE_OPTIONS First_Word_Fall_Through
  INPUT_DATA_WIDTH 64
  INPUT_DEPTH 16384
  OUTPUT_DATA_WIDTH 32
  OUTPUT_DEPTH 32768
  READ_DATA_COUNT true
  READ_DATA_COUNT_WIDTH 16
} {
  clk /ps_0/FCLK_CLK0
  srst slice_1/Dout
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo:1.0 fifo_1 {
  S_AXIS_TDATA_WIDTH 64
  M_AXIS_TDATA_WIDTH 32
} {
  S_AXIS comb_1/M_AXIS
  FIFO_READ fifo_generator_0/FIFO_READ
  FIFO_WRITE fifo_generator_0/FIFO_WRITE
  aclk /ps_0/FCLK_CLK0
}

# Create axi_axis_reader
cell pavel-demin:user:axi_axis_reader:1.0 reader_0 {
  AXI_DATA_WIDTH 32
} {
  S_AXIS fifo_1/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# TX

# Create axi_axis_writer
cell pavel-demin:user:axi_axis_writer:1.0 writer_0 {
  AXI_DATA_WIDTH 32
} {
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create fifo_generator
cell xilinx.com:ip:fifo_generator:13.0 fifo_generator_1 {
  PERFORMANCE_OPTIONS First_Word_Fall_Through
  INPUT_DATA_WIDTH 32
  INPUT_DEPTH 16384
  OUTPUT_DATA_WIDTH 32
  OUTPUT_DEPTH 16384
  DATA_COUNT true
  DATA_COUNT_WIDTH 15
} {
  clk /ps_0/FCLK_CLK0
  srst slice_2/Dout
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo:1.0 fifo_2 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
} {
  S_AXIS writer_0/M_AXIS
  FIFO_READ fifo_generator_1/FIFO_READ
  FIFO_WRITE fifo_generator_1/FIFO_WRITE
  aclk /ps_0/FCLK_CLK0
}

# Create axis_stepper
cell pavel-demin:user:axis_stepper:1.0 stepper_0 {
  AXIS_TDATA_WIDTH 32
} {
  S_AXIS fifo_2/M_AXIS
  trg_flag gen_1/out
  aclk ps_0/FCLK_CLK0
}

# Create axis_clock_converter
cell xilinx.com:ip:axis_clock_converter:1.1 fifo_3 {} {
  S_AXIS stepper_0/M_AXIS
  M_AXIS dac_0/S_AXIS
  s_axis_aclk ps_0/FCLK_CLK0
  s_axis_aresetn rst_0/peripheral_aresetn
  m_axis_aclk pll_0/clk_out1
  m_axis_aresetn const_1/dout
}

# STS

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_1 {
  NUM_PORTS 2
  IN0_WIDTH 16
  IN1_WIDTH 16
} {
  In0 fifo_generator_0/rd_data_count
  In1 fifo_generator_1/data_count
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
} [get_bd_intf_pins cfg_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_cfg_0_reg0]
set_property OFFSET 0x40000000 [get_bd_addr_segs ps_0/Data/SEG_cfg_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins sts_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_sts_0_reg0]
set_property OFFSET 0x40001000 [get_bd_addr_segs ps_0/Data/SEG_sts_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins reader_0/S_AXI]

set_property RANGE 128K [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]
set_property OFFSET 0x40020000 [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins writer_0/S_AXI]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]
set_property OFFSET 0x40010000 [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]
