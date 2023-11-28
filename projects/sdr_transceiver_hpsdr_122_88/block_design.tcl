# Create clk_wiz
cell xilinx.com:ip:clk_wiz pll_0 {
  PRIMITIVE PLL
  PRIM_IN_FREQ.VALUE_SRC USER
  PRIM_IN_FREQ 122.88
  PRIM_SOURCE Differential_clock_capable_pin
  CLKOUT1_USED true
  CLKOUT1_REQUESTED_OUT_FREQ 122.88
  CLKOUT2_USED true
  CLKOUT2_REQUESTED_OUT_FREQ 245.76
  CLKOUT2_REQUESTED_PHASE 157.5
  CLKOUT3_USED true
  CLKOUT3_REQUESTED_OUT_FREQ 245.76
  CLKOUT3_REQUESTED_PHASE 202.5
  USE_RESET false
} {
  clk_in1_p adc_clk_p_i
  clk_in1_n adc_clk_n_i
}

# Create processing_system7
cell xilinx.com:ip:processing_system7 ps_0 {
  PCW_IMPORT_BOARD_PRESET cfg/red_pitaya.xml
  PCW_USE_M_AXI_GP1 1
} {
  M_AXI_GP0_ACLK pll_0/clk_out1
  M_AXI_GP1_ACLK pll_0/clk_out1
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
  CFG_DATA_WIDTH 320
  STS_DATA_WIDTH 96
} {
  S_AXI ps_0/M_AXI_GP0
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
  ADC_DATA_WIDTH 16
} {
  aclk pll_0/clk_out1
  adc_dat_a adc_dat_a_i
  adc_dat_b adc_dat_b_i
  adc_csn adc_csn_o
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
  dac_clk dac_clk_o
  dac_rst dac_rst_o
  dac_sel dac_sel_o
  dac_wrt dac_wrt_o
  dac_dat dac_dat_o
  s_axis_tvalid const_0/dout
}

# GPIO

# Delete input/output port
delete_bd_objs [get_bd_ports exp_n_tri_io]

# Create input/output port
create_bd_port -dir IO -from 3 -to 0 exp_n_tri_io

# Create gpio_debouncer
cell pavel-demin:user:gpio_debouncer gpio_0 {
  DATA_WIDTH 4
  CNTR_WIDTH 16
} {
  gpio_data exp_n_tri_io
  aclk pll_0/clk_out1
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic not_0 {
  C_SIZE 4
  C_OPERATION not
} {
  Op1 gpio_0/deb_data
}

# Delete input/output port
delete_bd_objs [get_bd_ports exp_p_tri_io]

# Create output port
create_bd_port -dir O -from 7 -to 0 exp_p_tri_io

# Create port_slicer
cell pavel-demin:user:port_slicer out_slice_0 {
  DIN_WIDTH 320 DIN_FROM 31 DIN_TO 24
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer ptt_slice_0 {
  DIN_WIDTH 320 DIN_FROM 21 DIN_TO 21
} {
  din hub_0/cfg_data
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic or_0 {
  C_SIZE 1
  C_OPERATION or
} {
  Op1 ptt_slice_0/dout
  Op2 not_0/Res
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic or_1 {
  C_SIZE 8
  C_OPERATION or
} {
  Op1 out_slice_0/dout
  Op2 or_0/Res
  Res exp_p_tri_io
}

# ALEX

# Create output port
create_bd_port -dir IO -from 3 -to 0 exp_n_alex

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_0 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
  WRITE_DEPTH 1024
} {
  S_AXIS hub_0/M03_AXIS
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create axis_alex
cell pavel-demin:user:axis_alex alex_0 {} {
  S_AXIS fifo_0/M_AXIS
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# RX 0

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_0 {
  DIN_WIDTH 320 DIN_FROM 7 DIN_TO 0
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_1 {
  DIN_WIDTH 320 DIN_FROM 15 DIN_TO 8
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_0 {
  DIN_WIDTH 320 DIN_FROM 159 DIN_TO 32
} {
  din hub_0/cfg_data
}

module rx_0 {
  source projects/sdr_transceiver_hpsdr_122_88/rx.tcl
} {
  slice_0/din rst_slice_0/dout
  slice_1/din rst_slice_1/dout
  slice_2/din rst_slice_1/dout
  slice_3/din cfg_slice_0/dout
  slice_4/din cfg_slice_0/dout
  slice_5/din cfg_slice_0/dout
  slice_6/din cfg_slice_0/dout
  slice_7/din cfg_slice_0/dout
  slice_8/din cfg_slice_0/dout
  slice_9/din cfg_slice_0/dout
  slice_10/din cfg_slice_0/dout
}

# TX 0

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_2 {
  DIN_WIDTH 320 DIN_FROM 16 DIN_TO 16
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_3 {
  DIN_WIDTH 320 DIN_FROM 17 DIN_TO 17
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer key_slice_0 {
  DIN_WIDTH 320 DIN_FROM 18 DIN_TO 18
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer key_slice_1 {
  DIN_WIDTH 320 DIN_FROM 19 DIN_TO 19
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer sel_slice_0 {
  DIN_WIDTH 320 DIN_FROM 20 DIN_TO 20
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_1 {
  DIN_WIDTH 320 DIN_FROM 255 DIN_TO 160
} {
  din hub_0/cfg_data
}

module tx_0 {
  source projects/sdr_transceiver_hpsdr_122_88/tx.tcl
} {
  fifo_0/aresetn rst_slice_2/dout
  keyer_0/key_flag key_slice_0/dout
  sel_0/cfg_data sel_slice_0/dout
  slice_0/din rst_slice_1/dout
  slice_1/din cfg_slice_1/dout
  slice_2/din cfg_slice_1/dout
  slice_3/din cfg_slice_1/dout
  slice_4/din cfg_slice_1/dout
  slice_5/din cfg_slice_1/dout
  slice_6/din cfg_slice_1/dout
  dds_0/m_axis_data_tdata rx_0/dds_slice_6/din
  dds_0/m_axis_data_tdata rx_0/dds_slice_7/din
  dds_0/m_axis_data_tdata rx_0/dds_slice_8/din
  dds_0/m_axis_data_tdata rx_0/dds_slice_9/din
  concat_1/dout dac_0/s_axis_tdata
  mult_2/P rx_0/adc_slice_8/din
  mult_2/P rx_0/adc_slice_9/din
}

# CODEC

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_2 {
  DIN_WIDTH 320 DIN_FROM 319 DIN_TO 256
} {
  din hub_0/cfg_data
}

module codec {
  source projects/sdr_transceiver_hpsdr_122_88/codec.tcl
} {
  fifo_0/aresetn rst_slice_3/dout
  keyer_0/key_flag key_slice_1/dout
  slice_0/din rst_slice_0/dout
  slice_1/din rst_slice_0/dout
  slice_2/din cfg_slice_2/dout
  slice_3/din cfg_slice_2/dout
  slice_4/din cfg_slice_2/dout
  i2s_0/gpio_data exp_n_alex
  i2s_0/alex_data alex_0/alex_data
}

# STS

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  NUM_PORTS 5
  IN0_WIDTH 16
  IN1_WIDTH 16
  IN2_WIDTH 16
  IN3_WIDTH 16
  IN4_WIDTH 4
} {
  In0 rx_0/fifo_0/read_count
  In1 tx_0/fifo_0/write_count
  In2 codec/fifo_0/write_count
  In3 codec/fifo_1/read_count
  In4 not_0/Res
  dout hub_0/sts_data
}

wire rx_0/fifo_0/M_AXIS hub_0/S00_AXIS
wire tx_0/fifo_0/S_AXIS hub_0/M00_AXIS

wire codec/fifo_0/S_AXIS hub_0/M01_AXIS
wire codec/fifo_1/M_AXIS hub_0/S01_AXIS

wire tx_0/bram_0/BRAM_PORTA hub_0/B04_BRAM
wire codec/bram_0/BRAM_PORTA hub_0/B05_BRAM

# RX 1

module rx_1 {
  source projects/sdr_receiver_122_88/rx.tcl
} {
  hub_0/S_AXI ps_0/M_AXI_GP1
}
