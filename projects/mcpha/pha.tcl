# pha_0/aresetn

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0
}

# timer_0/aresetn

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_1 {
  DIN_WIDTH 8 DIN_FROM 1 DIN_TO 1
}

# pha_0/bln_flag

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_2 {
  DIN_WIDTH 8 DIN_FROM 2 DIN_TO 2
}

# timer_0/run_flag

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_3 {
  DIN_WIDTH 8 DIN_FROM 3 DIN_TO 3
}

# timer_0/cfg_data

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_4 {
  DIN_WIDTH 128 DIN_FROM 63 DIN_TO 0
}

# pha_0/bln_data

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_5 {
  DIN_WIDTH 128 DIN_FROM 79 DIN_TO 64
}

# pha_0/cfg_data

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_6 {
  DIN_WIDTH 128 DIN_FROM 95 DIN_TO 80
}

# pha_0/min_data

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_7 {
  DIN_WIDTH 128 DIN_FROM 111 DIN_TO 96
}

# pha_0/max_data

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_8 {
  DIN_WIDTH 128 DIN_FROM 127 DIN_TO 112
}

# Create axis_validator
cell pavel-demin:user:axis_timer:1.0 timer_0 {
  CNTR_WIDTH 64
} {
  run_flag slice_3/dout
  cfg_data slice_4/dout
  aclk /pll_0/clk_out1
  aresetn slice_1/dout
}
# Create axis_pulse_height_analyzer
cell pavel-demin:user:axis_pulse_height_analyzer:1.0 pha_0 {
  AXIS_TDATA_WIDTH 16
  AXIS_TDATA_SIGNED TRUE
  CNTR_WIDTH 16
} {
  bln_flag slice_2/dout
  bln_data slice_5/dout
  cfg_data slice_6/dout
  min_data slice_7/dout
  max_data slice_8/dout
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_validator
cell pavel-demin:user:axis_validator:1.0 vldtr_0 {
  AXIS_TDATA_WIDTH 16
} {
  S_AXIS pha_0/M_AXIS
  trg_flag timer_0/trg_flag
  aclk /pll_0/clk_out1
}
