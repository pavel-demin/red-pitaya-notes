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
cell xilinx.com:ip:clk_wiz:5.3 pll_0 {
  PRIM_IN_FREQ.VALUE_SRC USER
  PRIM_IN_FREQ 125.0
  CLKOUT1_USED true
  CLKOUT1_REQUESTED_OUT_FREQ 250.0
} {
  clk_in1 adc_0/adc_clk
}

# Create axis_zeroer
cell pavel-demin:user:axis_zeroer:1.0 zeroer_0 {
  AXIS_TDATA_WIDTH 32
} {
  aclk adc_0/adc_clk
}

# Create axis_red_pitaya_dac
cell pavel-demin:user:axis_red_pitaya_dac:1.0 dac_0 {} {
  aclk adc_0/adc_clk
  ddr_clk pll_0/clk_out1
  locked pll_0/locked
  S_AXIS zeroer_0/M_AXIS
  dac_clk dac_clk_o
  dac_rst dac_rst_o
  dac_sel dac_sel_o
  dac_wrt dac_wrt_o
  dac_dat dac_dat_o
}

# CFG

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register:1.0 cfg_0 {
  CFG_DATA_WIDTH 288
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0

# GPIO

# Delete input/output port
delete_bd_objs [get_bd_ports exp_p_tri_io]

# Create output port
create_bd_port -dir O -from 7 -to 0 exp_p_tri_io

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 out_slice_0 {
  DIN_WIDTH 288 DIN_FROM 23 DIN_TO 16 DOUT_WIDTH 8
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

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 pps_slice_0 {
  DIN_WIDTH 4 DIN_FROM 3 DIN_TO 3 DOUT_WIDTH 1
} {
  Din gpio_0/raw_data
  Dout ps_0/GPIO_I
}

# RX 0

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_0 {
  DIN_WIDTH 288 DIN_FROM 7 DIN_TO 0 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_0 {
  DIN_WIDTH 288 DIN_FROM 287 DIN_TO 32 DOUT_WIDTH 256
} {
  Din cfg_0/cfg_data
}

module rx_0 {
  source projects/sdr_transceiver_wspr/rx.tcl
} {
  slice_0/Din rst_slice_0/Dout
  slice_1/Din cfg_slice_0/Dout
  slice_2/Din cfg_slice_0/Dout
  slice_3/Din cfg_slice_0/Dout
  slice_4/Din cfg_slice_0/Dout
  slice_5/Din cfg_slice_0/Dout
  slice_6/Din cfg_slice_0/Dout
  slice_7/Din cfg_slice_0/Dout
  slice_8/Din cfg_slice_0/Dout
  fifo_0/S_AXIS adc_0/M_AXIS
  fifo_0/s_axis_aclk adc_0/adc_clk
  fifo_0/s_axis_aresetn const_0/dout
}

# TX 0

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_1 {
  DIN_WIDTH 288 DIN_FROM 15 DIN_TO 8 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

module tx_0 {
  source projects/sdr_transceiver_wspr/tx.tcl
} {
  slice_0/Din rst_slice_1/Dout
  fifo_1/M_AXIS zeroer_0/S_AXIS
  fifo_1/m_axis_aclk adc_0/adc_clk
  fifo_1/m_axis_aresetn const_0/dout
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
  In0 const_1/dout
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

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins rx_0/switch_0/S_AXI_CTRL]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_switch_0_Reg]
set_property OFFSET 0x40002000 [get_bd_addr_segs ps_0/Data/SEG_switch_0_Reg]

for {set i 0} {$i <= 7} {incr i} {

  # Create all required interconnections
  apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
    Master /ps_0/M_AXI_GP0
    Clk Auto
  } [get_bd_intf_pins rx_0/reader_$i/S_AXI]

  set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_reader_${i}_reg0]
  set_property OFFSET 0x4000[format %X [expr $i + 3]]000 [get_bd_addr_segs ps_0/Data/SEG_reader_${i}_reg0]

}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins tx_0/writer_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]
set_property OFFSET 0x4000B000 [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins tx_0/switch_0/S_AXI_CTRL]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_switch_0_Reg1]
set_property OFFSET 0x4000C000 [get_bd_addr_segs ps_0/Data/SEG_switch_0_Reg1]
