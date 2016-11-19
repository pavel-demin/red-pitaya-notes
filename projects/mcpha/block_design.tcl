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

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register:1.0 cfg_0 {
  CFG_DATA_WIDTH 448
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_0 {
  DIN_WIDTH 448 DIN_FROM 7 DIN_TO 0 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_1 {
  DIN_WIDTH 448 DIN_FROM 15 DIN_TO 8 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_2 {
  DIN_WIDTH 448 DIN_FROM 23 DIN_TO 16 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_3 {
  DIN_WIDTH 448 DIN_FROM 31 DIN_TO 24 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# rate_0/cfg_data and rate_1/cfg_data

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_1 {
  DIN_WIDTH 448 DIN_FROM 47 DIN_TO 32 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_0 {
  DIN_WIDTH 448 DIN_FROM 191 DIN_TO 64 DOUT_WIDTH 128
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_1 {
  DIN_WIDTH 448 DIN_FROM 319 DIN_TO 192 DOUT_WIDTH 128
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_2 {
  DIN_WIDTH 448 DIN_FROM 415 DIN_TO 320 DOUT_WIDTH 96
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_3 {
  DIN_WIDTH 448 DIN_FROM 447 DIN_TO 416 DOUT_WIDTH 32
} {
  Din cfg_0/cfg_data
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0

# Create axis_clock_converter
cell xilinx.com:ip:axis_clock_converter:1.1 fifo_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
} {
  S_AXIS adc_0/M_AXIS
  s_axis_aclk pll_0/clk_out1
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
  NUM_MI 4
  M00_TDATA_REMAP {tdata[15:0]}
  M01_TDATA_REMAP {tdata[31:16]}
  M02_TDATA_REMAP {16'b0000000000000000}
  M03_TDATA_REMAP {16'b0000000000000000}
} {
  S_AXIS fifo_0/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_variable
cell pavel-demin:user:axis_variable:1.0 rate_0 {
  AXIS_TDATA_WIDTH 16
} {
  cfg_data slice_1/Dout
  aclk /ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_variable
cell pavel-demin:user:axis_variable:1.0 rate_1 {
  AXIS_TDATA_WIDTH 16
} {
  cfg_data slice_1/Dout
  aclk /ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create cic_compiler
cell xilinx.com:ip:cic_compiler:4.0 cic_0 {
  INPUT_DATA_WIDTH.VALUE_SRC USER
  FILTER_TYPE Decimation
  NUMBER_OF_STAGES 6
  SAMPLE_RATE_CHANGES Programmable
  MINIMUM_RATE 4
  MAXIMUM_RATE 8192
  FIXED_OR_INITIAL_RATE 4
  INPUT_SAMPLE_FREQUENCY 125
  CLOCK_FREQUENCY 125
  INPUT_DATA_WIDTH 14
  QUANTIZATION Truncation
  OUTPUT_DATA_WIDTH 16
  USE_XTREME_DSP_SLICE false
  HAS_ARESETN true
} {
  S_AXIS_DATA bcast_0/M00_AXIS
  S_AXIS_CONFIG rate_0/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create cic_compiler
cell xilinx.com:ip:cic_compiler:4.0 cic_1 {
  INPUT_DATA_WIDTH.VALUE_SRC USER
  FILTER_TYPE Decimation
  NUMBER_OF_STAGES 6
  SAMPLE_RATE_CHANGES Programmable
  MINIMUM_RATE 4
  MAXIMUM_RATE 8192
  FIXED_OR_INITIAL_RATE 4
  INPUT_SAMPLE_FREQUENCY 125
  CLOCK_FREQUENCY 125
  INPUT_DATA_WIDTH 14
  QUANTIZATION Truncation
  OUTPUT_DATA_WIDTH 16
  USE_XTREME_DSP_SLICE false
  HAS_ARESETN true
} {
  S_AXIS_DATA bcast_0/M01_AXIS
  S_AXIS_CONFIG rate_1/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner:1.1 comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 2
} {
  S00_AXIS cic_0/M_AXIS_DATA
  S01_AXIS cic_1/M_AXIS_DATA
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create fir_compiler
cell xilinx.com:ip:fir_compiler:7.2 fir_0 {
  DATA_WIDTH.VALUE_SRC USER
  DATA_WIDTH 16
  COEFFICIENTVECTOR {4.1811671868e-06, 2.9962842696e-05, 1.2482197822e-04, 3.4115329109e-04, 7.6531605277e-04, 1.5146861420e-03, 2.7310518774e-03, 4.5689619595e-03, 7.1784709852e-03, 1.0683018711e-02, 1.5154640468e-02, 2.0589984382e-02, 2.6891362735e-02, 3.3857037591e-02, 4.1184036845e-02, 4.8485104800e-02, 5.5319177108e-02, 6.1232429042e-02, 6.5804926185e-02, 6.8696615948e-02, 6.9686119779e-02, 6.8696615948e-02, 6.5804926185e-02, 6.1232429042e-02, 5.5319177108e-02, 4.8485104800e-02, 4.1184036845e-02, 3.3857037591e-02, 2.6891362735e-02, 2.0589984382e-02, 1.5154640468e-02, 1.0683018711e-02, 7.1784709852e-03, 4.5689619595e-03, 2.7310518774e-03, 1.5146861420e-03, 7.6531605277e-04, 3.4115329109e-04, 1.2482197822e-04, 2.9962842696e-05, 4.1811671868e-06}
  COEFFICIENT_WIDTH 16
  QUANTIZATION Quantize_Only
  BESTPRECISION true
  NUMBER_PATHS 2
  SAMPLE_FREQUENCY 31.25
  CLOCK_FREQUENCY 125
  OUTPUT_ROUNDING_MODE Non_Symmetric_Rounding_Up
  OUTPUT_WIDTH 15
  HAS_ARESETN true
} {
  S_AXIS_DATA comb_0/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster:1.1 bcast_1 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 2
  NUM_MI 6
  M00_TDATA_REMAP {tdata[15:0]}
  M01_TDATA_REMAP {tdata[31:16]}
  M02_TDATA_REMAP {tdata[15:0]}
  M03_TDATA_REMAP {tdata[31:16]}
  M04_TDATA_REMAP {tdata[15:0]}
  M05_TDATA_REMAP {tdata[31:16]}
} {
  S_AXIS fir_0/M_AXIS_DATA
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

module hst_0 {
  source projects/mcpha/hst.tcl
} {
  slice_0/Din rst_slice_0/Dout
  slice_1/Din rst_slice_0/Dout
  slice_2/Din rst_slice_0/Dout
  slice_3/Din rst_slice_0/Dout
  slice_4/Din rst_slice_0/Dout
  slice_5/Din cfg_slice_0/Dout
  slice_6/Din cfg_slice_0/Dout
  slice_7/Din cfg_slice_0/Dout
  slice_8/Din cfg_slice_0/Dout
  slice_9/Din cfg_slice_0/Dout
  timer_0/S_AXIS bcast_0/M02_AXIS
  pha_0/S_AXIS bcast_1/M00_AXIS
}

module hst_1 {
  source projects/mcpha/hst.tcl
} {
  slice_0/Din rst_slice_1/Dout
  slice_1/Din rst_slice_1/Dout
  slice_2/Din rst_slice_1/Dout
  slice_3/Din rst_slice_1/Dout
  slice_4/Din rst_slice_1/Dout
  slice_5/Din cfg_slice_1/Dout
  slice_6/Din cfg_slice_1/Dout
  slice_7/Din cfg_slice_1/Dout
  slice_8/Din cfg_slice_1/Dout
  slice_9/Din cfg_slice_1/Dout
  timer_0/S_AXIS bcast_0/M03_AXIS
  pha_0/S_AXIS bcast_1/M01_AXIS
}

module osc_0 {
  source projects/mcpha/osc.tcl
} {
  slice_0/Din rst_slice_2/Dout
  slice_1/Din rst_slice_2/Dout
  slice_2/Din rst_slice_2/Dout
  slice_3/Din rst_slice_2/Dout
  slice_4/Din rst_slice_2/Dout
  slice_5/Din cfg_slice_2/Dout
  slice_6/Din cfg_slice_2/Dout
  slice_7/Din cfg_slice_2/Dout
  switch_0/S00_AXIS bcast_1/M02_AXIS
  switch_0/S01_AXIS bcast_1/M03_AXIS
  comb_0/S00_AXIS bcast_1/M04_AXIS
  comb_0/S01_AXIS bcast_1/M05_AXIS
  writer_0/M_AXI ps_0/S_AXI_HP0
}

module gen_0 {
  source projects/mcpha/gen.tcl
} {
  slice_0/Din rst_slice_3/Dout
  slice_1/Din cfg_slice_3/Dout
  fifo_1/M_AXIS dac_0/S_AXIS
  fifo_1/m_axis_aclk pll_0/clk_out1
  fifo_1/m_axis_aresetn const_0/dout
}

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_0 {
  NUM_PORTS 4
  IN0_WIDTH 64
  IN1_WIDTH 64
  IN2_WIDTH 32
  IN3_WIDTH 32
} {
  In0 hst_0/timer_0/sts_data
  In1 hst_1/timer_0/sts_data
  In2 osc_0/scope_0/sts_data
  In3 osc_0/writer_0/sts_data
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
}  [get_bd_intf_pins osc_0/switch_0/S_AXI_CTRL]

set_property RANGE 4K [get_bd_addr_segs {ps_0/Data/SEG_switch_0_Reg}]
set_property OFFSET 0x40002000 [get_bd_addr_segs {ps_0/Data/SEG_switch_0_Reg}]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins hst_0/reader_0/S_AXI]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]
set_property OFFSET 0x40010000 [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins hst_1/reader_0/S_AXI]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg01]
set_property OFFSET 0x40020000 [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg01]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins gen_0/writer_0/S_AXI]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]
set_property OFFSET 0x40030000 [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]

assign_bd_address [get_bd_addr_segs ps_0/S_AXI_HP0/HP0_DDR_LOWOCM]
