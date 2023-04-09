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
}

# HUB

# Create axi_hub
cell pavel-demin:user:axi_hub hub_0 {
  CFG_DATA_WIDTH 352
  STS_DATA_WIDTH 32
} {
  S_AXI ps_0/M_AXI_GP0
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 352 DIN_FROM 0 DIN_TO 0
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 352 DIN_FROM 1 DIN_TO 1
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 352 DIN_FROM 2 DIN_TO 2
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_4 {
  DIN_WIDTH 352 DIN_FROM 3 DIN_TO 3
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_5 {
  DIN_WIDTH 352 DIN_FROM 8 DIN_TO 8
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_6 {
  DIN_WIDTH 352 DIN_FROM 23 DIN_TO 16
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_7 {
  DIN_WIDTH 352 DIN_FROM 31 DIN_TO 24
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_8 {
  DIN_WIDTH 352 DIN_FROM 39 DIN_TO 32
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_9 {
  DIN_WIDTH 352 DIN_FROM 47 DIN_TO 40
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_10 {
  DIN_WIDTH 352 DIN_FROM 159 DIN_TO 64
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_11 {
  DIN_WIDTH 352 DIN_FROM 255 DIN_TO 160
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_12 {
  DIN_WIDTH 352 DIN_FROM 351 DIN_TO 256
} {
  din hub_0/cfg_data
}

# Pulse generators

# Create trigger pulse generator
cell pavel-demin:user:pulse_generator gen_0 {
  CONTINUOUS TRUE
} {
  cfg slice_10/dout
  aclk pll_0/clk_out1
  aresetn slice_1/dout
}

# Create S&H pulse generator
cell pavel-demin:user:pulse_generator gen_1 {
  CONTINUOUS TRUE
} {
  cfg slice_11/dout
  aclk pll_0/clk_out1
  aresetn slice_1/dout
}

# Create axis_gpio_reader
cell pavel-demin:user:axis_gpio_reader gpio_0 {
  AXIS_TDATA_WIDTH 8
} {
  gpio_data exp_n_tri_io
  aclk pll_0/clk_out1
}

# Create axis_trigger
cell pavel-demin:user:axis_trigger trig_0 {
  AXIS_TDATA_WIDTH 8
  AXIS_TDATA_SIGNED FALSE
} {
  S_AXIS gpio_0/M_AXIS
  pol_data slice_5/dout
  msk_data slice_6/dout
  lvl_data slice_7/dout
  aclk pll_0/clk_out1
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic not_0 {
  C_SIZE 1
  C_OPERATION not
} {
  Op1 trig_0/trg_flag
}

# Create ADC pulse generator
cell pavel-demin:user:pulse_generator gen_2 {
  CONTINUOUS FALSE
} {
  cfg slice_12/dout
  aclk pll_0/clk_out1
  aresetn not_0/Res
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic xor_0 {
  C_SIZE 1
  C_OPERATION xor
} {
  Op1 slice_3/dout
  Op2 gen_0/dout
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic xor_1 {
  C_SIZE 1
  C_OPERATION xor
} {
  Op1 slice_4/dout
  Op2 gen_1/dout
}

# Delete input/output port
delete_bd_objs [get_bd_ports exp_p_tri_io]

# Create output port
create_bd_port -dir O -from 7 -to 0 exp_p_tri_io

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  NUM_PORTS 4
  IN0_WIDTH 1
  IN1_WIDTH 1
  IN2_WIDTH 1
} {
  In0 xor_0/Res
  In1 xor_1/Res
  In2 gen_2/dout
  dout exp_p_tri_io
}

# Scan control

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_0 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
  WRITE_DEPTH 16384
} {
  S_AXIS hub_0/M00_AXIS
  aclk pll_0/clk_out1
  aresetn slice_2/dout
}

# Create axis_stepper
cell pavel-demin:user:axis_stepper stepper_0 {
  AXIS_TDATA_WIDTH 32
} {
  S_AXIS fifo_0/M_AXIS
  M_AXIS dac_0/S_AXIS
  trg_flag trig_0/trg_flag
  aclk pll_0/clk_out1
}

# Acquisition

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster bcast_0 {
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
cell xilinx.com:ip:util_vector_logic not_1 {
  C_SIZE 1
  C_OPERATION not
} {
  Op1 gen_2/dout
}

# Create axis_variable
cell pavel-demin:user:axis_accumulator accu_0 {
  S_AXIS_TDATA_WIDTH 16
  M_AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 8
  AXIS_TDATA_SIGNED TRUE
  CONTINUOUS FALSE
} {
  S_AXIS bcast_0/M00_AXIS
  cfg_data slice_8/dout
  aclk pll_0/clk_out1
  aresetn not_1/Res
}

# Create axis_variable
cell pavel-demin:user:axis_accumulator accu_1 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 8
  AXIS_TDATA_SIGNED TRUE
  CONTINUOUS TRUE
} {
  S_AXIS accu_0/M_AXIS
  cfg_data slice_9/dout
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create axis_variable
cell pavel-demin:user:axis_accumulator accu_2 {
  S_AXIS_TDATA_WIDTH 16
  M_AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 8
  AXIS_TDATA_SIGNED TRUE
  CONTINUOUS FALSE
} {
  S_AXIS bcast_0/M01_AXIS
  cfg_data slice_8/dout
  aclk pll_0/clk_out1
  aresetn not_1/Res
}

# Create axis_variable
cell pavel-demin:user:axis_accumulator accu_3 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 8
  AXIS_TDATA_SIGNED TRUE
  CONTINUOUS TRUE
} {
  S_AXIS accu_2/M_AXIS
  cfg_data slice_9/dout
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
} {
  S00_AXIS accu_1/M_AXIS
  S01_AXIS accu_3/M_AXIS
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_1 {
  S_AXIS_TDATA_WIDTH 64
  M_AXIS_TDATA_WIDTH 32
  WRITE_DEPTH 16384
} {
  S_AXIS comb_0/M_AXIS
  M_AXIS hub_0/S00_AXIS
  aclk pll_0/clk_out1
  aresetn slice_2/dout
}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_1 {
  NUM_PORTS 2
  IN0_WIDTH 16
  IN1_WIDTH 16
} {
  In0 fifo_0/write_count
  In1 fifo_1/read_count
  dout hub_0/sts_data
}
