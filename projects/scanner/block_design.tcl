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

# ADC

# Create axis_red_pitaya_adc
cell pavel-demin:user:axis_red_pitaya_adc:2.0 adc_0 {} {
  aclk pll_0/clk_out1
  adc_dat_a adc_dat_a_i
  adc_dat_b adc_dat_b_i
  adc_csn adc_csn_o
}

# DAC

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
  CFG_DATA_WIDTH 416
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_1 {
  DIN_WIDTH 416 DIN_FROM 0 DIN_TO 0 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_2 {
  DIN_WIDTH 416 DIN_FROM 1 DIN_TO 1 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_3 {
  DIN_WIDTH 416 DIN_FROM 2 DIN_TO 2 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_4 {
  DIN_WIDTH 416 DIN_FROM 3 DIN_TO 3 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_5 {
  DIN_WIDTH 416 DIN_FROM 23 DIN_TO 16 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_6 {
  DIN_WIDTH 416 DIN_FROM 31 DIN_TO 24 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_7 {
  DIN_WIDTH 416 DIN_FROM 127 DIN_TO 32 DOUT_WIDTH 96
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_8 {
  DIN_WIDTH 416 DIN_FROM 223 DIN_TO 128 DOUT_WIDTH 96
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_9 {
  DIN_WIDTH 416 DIN_FROM 319 DIN_TO 224 DOUT_WIDTH 96
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_10 {
  DIN_WIDTH 416 DIN_FROM 415 DIN_TO 320 DOUT_WIDTH 96
} {
  Din cfg_0/cfg_data
}

# Pulse generators

# Create axis_packetizer
cell pavel-demin:user:pulse_generator:1.0 gen_0 {} {
  cfg slice_7/Dout
  aclk pll_0/clk_out1
  aresetn slice_1/Dout
}

# Create axis_packetizer
cell pavel-demin:user:pulse_generator:1.0 gen_1 {} {
  cfg slice_8/Dout
  aclk pll_0/clk_out1
  aresetn slice_1/Dout
}

# Create axis_packetizer
cell pavel-demin:user:pulse_generator:1.0 gen_2 {} {
  cfg slice_9/Dout
  aclk pll_0/clk_out1
  aresetn slice_1/Dout
}

# Create axis_packetizer
cell pavel-demin:user:pulse_generator:1.0 gen_3 {} {
  cfg slice_10/Dout
  aclk pll_0/clk_out1
  aresetn slice_1/Dout
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic:2.0 xor_0 {
  C_SIZE 1
  C_OPERATION xor
} {
  Op1 slice_3/Dout
  Op2 gen_0/dout
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic:2.0 xor_1 {
  C_SIZE 1
  C_OPERATION xor
} {
  Op1 slice_4/Dout
  Op2 gen_1/dout
}

# Delete input/output port
delete_bd_objs [get_bd_ports exp_p_tri_io]

# Create output port
create_bd_port -dir O -from 7 -to 0 exp_p_tri_io

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_0 {
  NUM_PORTS 4
  IN0_WIDTH 1
  IN1_WIDTH 1
  IN2_WIDTH 1
  IN3_WIDTH 1
} {
  In0 xor_0/Res
  In1 xor_1/Res
  In2 gen_2/dout
  In3 gen_3/dout
  dout exp_p_tri_io
}

# Scan control

# Create axi_axis_writer
cell pavel-demin:user:axi_axis_writer:1.0 writer_0 {
  AXI_DATA_WIDTH 32
} {
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create fifo_generator
cell xilinx.com:ip:fifo_generator:13.1 fifo_generator_0 {
  PERFORMANCE_OPTIONS First_Word_Fall_Through
  INPUT_DATA_WIDTH 32
  INPUT_DEPTH 16384
  OUTPUT_DATA_WIDTH 32
  OUTPUT_DEPTH 16384
  DATA_COUNT true
  DATA_COUNT_WIDTH 15
} {
  clk /pll_0/clk_out1
  srst slice_2/Dout
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo:1.0 fifo_0 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
} {
  S_AXIS writer_0/M_AXIS
  FIFO_READ fifo_generator_0/FIFO_READ
  FIFO_WRITE fifo_generator_0/FIFO_WRITE
  aclk /pll_0/clk_out1
}

# Create axis_stepper
cell pavel-demin:user:axis_stepper:1.0 stepper_0 {
  AXIS_TDATA_WIDTH 32
} {
  S_AXIS fifo_0/M_AXIS
  M_AXIS dac_0/S_AXIS
  trg_flag gen_2/dout
  aclk pll_0/clk_out1
}

# Acquisition

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster:1.1 bcast_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 2
  M00_TDATA_REMAP {tdata[15:0]}
  M01_TDATA_REMAP {tdata[31:16]}
} {
  S_AXIS adc_0/M_AXIS
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic:2.0 not_0 {
  C_SIZE 1
  C_OPERATION not
} {
  Op1 gen_3/dout
}

# Create axis_variable
cell pavel-demin:user:axis_accumulator:1.0 accu_0 {
  S_AXIS_TDATA_WIDTH 16
  M_AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 8
  AXIS_TDATA_SIGNED TRUE
  CONTINUOUS FALSE
} {
  S_AXIS bcast_0/M00_AXIS
  cfg_data slice_5/Dout
  aclk pll_0/clk_out1
  aresetn not_0/Res
}

# Create axis_variable
cell pavel-demin:user:axis_accumulator:1.0 accu_1 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 8
  AXIS_TDATA_SIGNED TRUE
  CONTINUOUS TRUE
} {
  S_AXIS accu_0/M_AXIS
  cfg_data slice_6/Dout
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create axis_variable
cell pavel-demin:user:axis_accumulator:1.0 accu_2 {
  S_AXIS_TDATA_WIDTH 16
  M_AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 8
  AXIS_TDATA_SIGNED TRUE
  CONTINUOUS FALSE
} {
  S_AXIS bcast_0/M01_AXIS
  cfg_data slice_5/Dout
  aclk pll_0/clk_out1
  aresetn not_0/Res
}

# Create axis_variable
cell pavel-demin:user:axis_accumulator:1.0 accu_3 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 8
  AXIS_TDATA_SIGNED TRUE
  CONTINUOUS TRUE
} {
  S_AXIS accu_2/M_AXIS
  cfg_data slice_6/Dout
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner:1.1 comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
} {
  S00_AXIS accu_3/M_AXIS
  S01_AXIS accu_1/M_AXIS
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create fifo_generator
cell xilinx.com:ip:fifo_generator:13.1 fifo_generator_1 {
  PERFORMANCE_OPTIONS First_Word_Fall_Through
  INPUT_DATA_WIDTH 64
  INPUT_DEPTH 16384
  OUTPUT_DATA_WIDTH 32
  OUTPUT_DEPTH 32768
  READ_DATA_COUNT true
  READ_DATA_COUNT_WIDTH 16
} {
  clk /pll_0/clk_out1
  srst slice_2/Dout
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo:1.0 fifo_3 {
  S_AXIS_TDATA_WIDTH 64
  M_AXIS_TDATA_WIDTH 32
} {
  S_AXIS comb_0/M_AXIS
  FIFO_READ fifo_generator_1/FIFO_READ
  FIFO_WRITE fifo_generator_1/FIFO_WRITE
  aclk /pll_0/clk_out1
}

# Create axi_axis_reader
cell pavel-demin:user:axi_axis_reader:1.0 reader_0 {
  AXI_DATA_WIDTH 32
} {
  S_AXIS fifo_3/M_AXIS
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# STS

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_1 {
  NUM_PORTS 2
  IN0_WIDTH 16
  IN1_WIDTH 16
} {
  In0 fifo_generator_0/data_count
  In1 fifo_generator_1/rd_data_count
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
} [get_bd_intf_pins writer_0/S_AXI]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]
set_property OFFSET 0x40010000 [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins reader_0/S_AXI]

set_property RANGE 128K [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]
set_property OFFSET 0x40020000 [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]
