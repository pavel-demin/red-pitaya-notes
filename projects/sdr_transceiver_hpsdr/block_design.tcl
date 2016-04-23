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
  CLKOUT1_REQUESTED_OUT_FREQ 250.0
} {
  clk_in1 adc_0/adc_clk
}

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
  aclk adc_0/adc_clk
  aresetn const_0/dout
}

# Create axis_red_pitaya_dac
cell pavel-demin:user:axis_red_pitaya_dac:1.0 dac_0 {} {
  aclk adc_0/adc_clk
  ddr_clk pll_0/clk_out1
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
  CFG_DATA_WIDTH 288
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
  DIN_WIDTH 288 DIN_FROM 23 DIN_TO 16 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
  Dout exp_p_tri_io
}

# Create gpio_debouncer
cell pavel-demin:user:gpio_debouncer:1.0 gpio_0 {
  DATA_WIDTH 4
  CNTR_WIDTH 22
} {
  gpio_data exp_n_tri_io
  aclk ps_0/FCLK_CLK0
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic:2.0 not_0 {
  C_SIZE 4
  C_OPERATION not
} {
  Op1 gpio_0/out
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
  aclk adc_0/adc_clk
  aresetn const_0/dout
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_0 {
  DIN_WIDTH 288 DIN_FROM 7 DIN_TO 0 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_0 {
  DIN_WIDTH 288 DIN_FROM 191 DIN_TO 32 DOUT_WIDTH 160
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

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_1 {
  DIN_WIDTH 288 DIN_FROM 287 DIN_TO 192 DOUT_WIDTH 96
} {
  Din cfg_0/cfg_data
}

module tx_0 {
  source projects/sdr_transceiver_hpsdr/tx.tcl
} {
  slice_0/Din rst_slice_1/Dout
  slice_1/Din rst_slice_1/Dout
  slice_2/Din cfg_slice_1/Dout
  slice_3/Din cfg_slice_1/Dout
  fifo_1/M_AXIS bcast_0/S_AXIS
  fifo_1/m_axis_aclk adc_0/adc_clk
  fifo_1/m_axis_aresetn const_0/dout
}

# STS

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_2

# Create dna_reader
cell pavel-demin:user:dna_reader:1.0 dna_0 {} {
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_0 {
  NUM_PORTS 5
  IN0_WIDTH 32
  IN1_WIDTH 64
  IN2_WIDTH 16
  IN3_WIDTH 16
  IN4_WIDTH 4
} {
  In0 const_2/dout
  In1 dna_0/dna_data
  In2 rx_0/fifo_generator_0/rd_data_count
  In3 tx_0/fifo_generator_0/data_count
  In4 not_0/Res
}

# Create axi_sts_register
cell pavel-demin:user:axi_sts_register:1.0 sts_0 {
  STS_DATA_WIDTH 160
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
} [get_bd_intf_pins rx_0/reader_0/S_AXI]

set_property RANGE 32K [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]
set_property OFFSET 0x40008000 [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins tx_0/writer_0/S_AXI]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]
set_property OFFSET 0x40010000 [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]
