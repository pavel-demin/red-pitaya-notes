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
  dcm_locked pll_0/locked
  slowest_sync_clk pll_0/clk_out1
}

# HUB

# Create axi_hub
cell pavel-demin:user:axi_hub hub_0 {
  CFG_DATA_WIDTH 64
  STS_DATA_WIDTH 32
} {
  S_AXI ps_0/M_AXI_GP0
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 64 DIN_FROM 0 DIN_TO 0
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 64 DIN_FROM 1 DIN_TO 1
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 64 DIN_FROM 2 DIN_TO 2
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 64 DIN_FROM 63 DIN_TO 32
} {
  din hub_0/cfg_data
}

# FIFO

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_0 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 128
  WRITE_DEPTH 1024
} {
  S_AXIS hub_0/M00_AXIS
  aclk pll_0/clk_out1
  aresetn slice_0/dout
}

# GATE

# Create axis_gate_controller
cell pavel-demin:user:axis_gate_controller gate_0 {} {
  S_AXIS fifo_0/M_AXIS
  aclk pll_0/clk_out1
  aresetn slice_1/dout
}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  NUM_PORTS 2
  IN0_WIDTH 32
  IN1_WIDTH 32
} {
  In0 slice_3/dout
  In1 gate_0/poff
}

# Create axis_constant
cell pavel-demin:user:axis_constant phase_0 {
  AXIS_TDATA_WIDTH 64
} {
  cfg_data concat_0/dout
  aclk pll_0/clk_out1
}

# Create dds_compiler
cell xilinx.com:ip:dds_compiler dds_0 {
  DDS_CLOCK_RATE 125
  SPURIOUS_FREE_DYNAMIC_RANGE 138
  FREQUENCY_RESOLUTION 0.2
  PHASE_INCREMENT Streaming
  PHASE_OFFSET Streaming
  HAS_ARESETN true
  HAS_PHASE_OUT false
  PHASE_WIDTH 30
  OUTPUT_WIDTH 24
  DSP48_USE Minimal
  OUTPUT_SELECTION Sine
} {
  S_AXIS_PHASE phase_0/M_AXIS
  aclk pll_0/clk_out1
  aresetn slice_1/dout
}

# Create c_shift_ram
cell xilinx.com:ip:c_shift_ram delay_0 {
  WIDTH.VALUE_SRC USER
  WIDTH 16
  DEPTH 11
} {
  D gate_0/level
  CLK pll_0/clk_out1
}

# Create dsp48
cell pavel-demin:user:dsp48 mult_0 {
  A_WIDTH 24
  B_WIDTH 16
  P_WIDTH 16
} {
  A dds_0/m_axis_data_tdata
  B delay_0/Q
  CLK pll_0/clk_out1
}

# Create c_shift_ram
cell xilinx.com:ip:c_shift_ram delay_1 {
  WIDTH.VALUE_SRC USER
  WIDTH 1
  DEPTH 14
} {
  D gate_0/dout
  CLK pll_0/clk_out1
}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_1 {
  NUM_PORTS 3
  IN0_WIDTH 16
  IN1_WIDTH 8
  IN2_WIDTH 1
} {
  In0 mult_0/P
  In1 delay_1/Q
  In2 gate_0/dout
}

# Create axis_constant
cell pavel-demin:user:axis_constant output_0 {
  AXIS_TDATA_WIDTH 32
} {
  cfg_data concat_1/dout
  aclk pll_0/clk_out1
}

# FIFO

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_1 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
  WRITE_DEPTH 1024
} {
  S_AXIS output_0/M_AXIS
  M_AXIS hub_0/S00_AXIS
  read_count hub_0/sts_data
  aclk pll_0/clk_out1
  aresetn slice_2/dout
}
