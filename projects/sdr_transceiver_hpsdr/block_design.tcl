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

# XADC

# Create xadc_wiz
cell xilinx.com:ip:xadc_wiz:3.3 xadc_0 {
  DCLK_FREQUENCY 143
  ADC_CONVERSION_RATE 100
  XADC_STARUP_SELECTION independent_adc
  CHANNEL_ENABLE_VAUXP0_VAUXN0 true
  CHANNEL_ENABLE_VAUXP1_VAUXN1 true
  CHANNEL_ENABLE_VAUXP8_VAUXN8 true
  CHANNEL_ENABLE_VAUXP9_VAUXN9 true
  CHANNEL_ENABLE_VP_VN true
} {
  Vp_Vn Vp_Vn
  Vaux0 Vaux0
  Vaux1 Vaux1
  Vaux8 Vaux8
  Vaux9 Vaux9
}

# PLL

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

# ADC

# Create axis_red_pitaya_adc
cell pavel-demin:user:axis_red_pitaya_adc:2.0 adc_0 {} {
  aclk pll_0/clk_out1
  adc_dat_a adc_dat_a_i
  adc_dat_b adc_dat_b_i
  adc_csn adc_csn_o
}

# DAC

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster:1.1 bcast_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 2
  M_TDATA_NUM_BYTES 2
  M00_TDATA_REMAP {tdata[15:0]}
  M01_TDATA_REMAP {tdata[15:0]}
} {
  aclk pll_0/clk_out1
  aresetn const_0/dout
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
  S_AXIS bcast_0/M00_AXIS
}

# CFG

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register:1.0 cfg_0 {
  CFG_DATA_WIDTH 320
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

# GPIO

# Delete input/output port
delete_bd_objs [get_bd_ports exp_p_tri_io]

# Create output port
create_bd_port -dir O -from 7 -to 0 exp_p_tri_io

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 out_slice_0 {
  DIN_WIDTH 320 DIN_FROM 31 DIN_TO 24 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
  Dout exp_p_tri_io
}

# Delete input/output port
delete_bd_objs [get_bd_ports exp_n_tri_io]

# Create input/output port
create_bd_port -dir IO -from 3 -to 0 exp_n_tri_io

# Create gpio_debouncer
cell pavel-demin:user:gpio_debouncer:1.0 gpio_0 {
  DATA_WIDTH 4
  CNTR_WIDTH 16
} {
  gpio_data exp_n_tri_io
  aclk ps_0/FCLK_CLK0
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic:2.0 not_0 {
  C_SIZE 4
  C_OPERATION not
} {
  Op1 gpio_0/deb_data
}

# ALEX

# Create output port
create_bd_port -dir IO -from 3 -to 0 exp_n_alex

# Create axi_axis_writer
cell pavel-demin:user:axi_axis_writer:1.0 writer_0 {
  AXI_DATA_WIDTH 32
} {
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_data_fifo
cell xilinx.com:ip:axis_data_fifo:1.1 fifo_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
  FIFO_DEPTH 1024
} {
  S_AXIS writer_0/M_AXIS
  s_axis_aclk ps_0/FCLK_CLK0
  s_axis_aresetn rst_0/peripheral_aresetn
}

# Create axis_alex
cell pavel-demin:user:axis_alex:1.0 alex_0 {} {
  S_AXIS fifo_0/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# RX 0

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner:1.1 comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
  NUM_SI 2
} {
  S00_AXIS adc_0/M_AXIS
  S01_AXIS bcast_0/M01_AXIS
  aclk pll_0/clk_out1
  aresetn const_0/dout
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_0 {
  DIN_WIDTH 320 DIN_FROM 7 DIN_TO 0 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_0 {
  DIN_WIDTH 320 DIN_FROM 191 DIN_TO 32 DOUT_WIDTH 160
} {
  Din cfg_0/cfg_data
}

module rx_0 {
  source projects/sdr_transceiver_hpsdr/rx.tcl
} {
  slice_0/Din rst_slice_0/Dout
  slice_1/Din cfg_slice_0/Dout
  slice_2/Din cfg_slice_0/Dout
  slice_3/Din cfg_slice_0/Dout
  slice_4/Din cfg_slice_0/Dout
  slice_5/Din cfg_slice_0/Dout
  fifo_0/S_AXIS comb_0/M_AXIS
  fifo_0/s_axis_aclk pll_0/clk_out1
  fifo_0/s_axis_aresetn const_0/dout
}

# TX 0

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_1 {
  DIN_WIDTH 320 DIN_FROM 15 DIN_TO 8 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_1 {
  DIN_WIDTH 320 DIN_FROM 255 DIN_TO 192 DOUT_WIDTH 64
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 key_slice_0 {
  DIN_WIDTH 4 DIN_FROM 2 DIN_TO 2 DOUT_WIDTH 1
} {
  Din not_0/Res
}

module tx_0 {
  source projects/sdr_transceiver_hpsdr/tx.tcl
} {
  slice_0/Din rst_slice_1/Dout
  slice_1/Din rst_slice_1/Dout
  slice_2/Din cfg_slice_1/Dout
  slice_3/Din cfg_slice_1/Dout
  slice_4/Din cfg_slice_1/Dout
  keyer_0/key_flag key_slice_0/Dout
  fifo_1/M_AXIS bcast_0/S_AXIS
  fifo_1/m_axis_aclk pll_0/clk_out1
  fifo_1/m_axis_aresetn const_0/dout
}

# CODEC

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_2 {
  DIN_WIDTH 320 DIN_FROM 23 DIN_TO 16 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_2 {
  DIN_WIDTH 320 DIN_FROM 319 DIN_TO 256 DOUT_WIDTH 64
} {
  Din cfg_0/cfg_data
}

module codec {
  source projects/sdr_transceiver_hpsdr/codec.tcl
} {
  slice_0/Din rst_slice_2/Dout
  slice_1/Din rst_slice_2/Dout
  slice_2/Din rst_slice_2/Dout
  slice_3/Din rst_slice_2/Dout
  slice_4/Din cfg_slice_2/Dout
  slice_5/Din cfg_slice_2/Dout
  slice_6/Din cfg_slice_2/Dout
  keyer_0/key_flag key_slice_0/Dout
  i2s_0/gpio_data exp_n_alex
  i2s_0/alex_data alex_0/alex_data
}

# STS

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_1

# Create dna_reader
cell pavel-demin:user:dna_reader:1.0 dna_0 {} {
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_0 {
  NUM_PORTS 7
  IN0_WIDTH 32
  IN1_WIDTH 64
  IN2_WIDTH 16
  IN3_WIDTH 16
  IN4_WIDTH 16
  IN5_WIDTH 16
  IN6_WIDTH 4
} {
  In0 const_1/dout
  In1 dna_0/dna_data
  In2 rx_0/fifo_generator_0/rd_data_count
  In3 tx_0/fifo_generator_0/data_count
  In4 codec/fifo_generator_0/data_count
  In5 codec/fifo_generator_1/data_count
  In6 not_0/Res
}

# Create axi_sts_register
cell pavel-demin:user:axi_sts_register:1.0 sts_0 {
  STS_DATA_WIDTH 192
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

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins writer_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]
set_property OFFSET 0x40002000 [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins rx_0/reader_0/S_AXI]

set_property RANGE 32K [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]
set_property OFFSET 0x40010000 [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins tx_0/writer_0/S_AXI]

set_property RANGE 8K [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg01]
set_property OFFSET 0x4000A000 [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg01]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins tx_0/switch_0/S_AXI_CTRL]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_switch_0_Reg]
set_property OFFSET 0x40003000 [get_bd_addr_segs ps_0/Data/SEG_switch_0_Reg]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins tx_0/writer_1/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_writer_1_reg0]
set_property OFFSET 0x40004000 [get_bd_addr_segs ps_0/Data/SEG_writer_1_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins codec/switch_0/S_AXI_CTRL]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_switch_0_Reg1]
set_property OFFSET 0x40005000 [get_bd_addr_segs ps_0/Data/SEG_switch_0_Reg1]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins codec/writer_1/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_writer_1_reg01]
set_property OFFSET 0x40006000 [get_bd_addr_segs ps_0/Data/SEG_writer_1_reg01]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins codec/writer_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg02]
set_property OFFSET 0x40007000 [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg02]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins codec/reader_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg01]
set_property OFFSET 0x40008000 [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg01]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins xadc_0/s_axi_lite]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_xadc_0_Reg]
set_property OFFSET 0x40020000 [get_bd_addr_segs ps_0/Data/SEG_xadc_0_Reg]
