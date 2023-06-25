# pha_0/aresetn

# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0
}

# timer_0/aresetn

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 8 DIN_FROM 1 DIN_TO 1
}

# timer_0/run_flag

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 8 DIN_FROM 2 DIN_TO 2
}

# timer_0/cfg_data

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 128 DIN_FROM 63 DIN_TO 0
}

# pha_0/cfg_data

# Create port_slicer
cell pavel-demin:user:port_slicer slice_4 {
  DIN_WIDTH 128 DIN_FROM 79 DIN_TO 64
}

# pha_0/min_data

# Create port_slicer
cell pavel-demin:user:port_slicer slice_5 {
  DIN_WIDTH 128 DIN_FROM 95 DIN_TO 80
}

# pha_0/max_data

# Create port_slicer
cell pavel-demin:user:port_slicer slice_6 {
  DIN_WIDTH 128 DIN_FROM 111 DIN_TO 96
}

# Create axis_timer
cell pavel-demin:user:axis_timer timer_0 {
  CNTR_WIDTH 64
} {
  run_flag slice_2/dout
  cfg_data slice_3/dout
  aclk /pll_0/clk_out1
  aresetn slice_1/dout
}

# Create axis_pulse_height_analyzer
cell pavel-demin:user:axis_pulse_height_analyzer pha_0 {
  AXIS_TDATA_WIDTH 16
  AXIS_TDATA_SIGNED TRUE
  CNTR_WIDTH 16
} {
  cfg_data slice_4/dout
  min_data slice_5/dout
  max_data slice_6/dout
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_validator
cell pavel-demin:user:axis_validator vldtr_0 {
  AXIS_TDATA_WIDTH 16
} {
  S_AXIS pha_0/M_AXIS
  trg_flag timer_0/trg_flag
  aclk /pll_0/clk_out1
}
