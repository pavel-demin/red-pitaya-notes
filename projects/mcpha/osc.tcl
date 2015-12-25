# scope_0/aresetn

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0 DOUT_WIDTH 1
}

# conv_0/aresetn and writer_0/aresetn

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_1 {
  DIN_WIDTH 8 DIN_FROM 1 DIN_TO 1 DOUT_WIDTH 1
}

# trig_0/pol_data

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_2 {
  DIN_WIDTH 8 DIN_FROM 2 DIN_TO 2 DOUT_WIDTH 1
}

# scope_0/run_flag

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_3 {
  DIN_WIDTH 8 DIN_FROM 3 DIN_TO 3 DOUT_WIDTH 1
}

# scope_0/pre_data

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_4 {
  DIN_WIDTH 96 DIN_FROM 31 DIN_TO 0 DOUT_WIDTH 32
}

# scope_0/tot_data

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_5 {
  DIN_WIDTH 96 DIN_FROM 63 DIN_TO 32 DOUT_WIDTH 32
}

# trig_0/lvl_data

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_6 {
  DIN_WIDTH 96 DIN_FROM 79 DIN_TO 64 DOUT_WIDTH 16
}

# Create axis_switch
cell xilinx.com:ip:axis_switch:1.1 switch_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 2
  ROUTING_MODE 1
} {
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0 {
  CONST_WIDTH 14
  CONST_VAL 16383
}

# Create axis_trigger
cell pavel-demin:user:axis_trigger:1.0 trig_0 {
  AXIS_TDATA_WIDTH 14
  AXIS_TDATA_SIGNED FALSE
} {
  S_AXIS switch_0/M00_AXIS
  pol_data slice_2/Dout
  msk_data const_0/dout
  lvl_data slice_6/Dout
  aclk /ps_0/FCLK_CLK0
}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner:1.1 comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 2
} {
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_oscilloscope
cell pavel-demin:user:axis_oscilloscope:1.0 scope_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 23
} {
  S_AXIS comb_0/M_AXIS
  run_flag slice_3/Dout
  trg_flag trig_0/trg_flag
  pre_data slice_4/Dout
  tot_data slice_5/Dout
  aclk /ps_0/FCLK_CLK0
  aresetn slice_0/Dout
}

# Create axis_dwidth_converter
cell xilinx.com:ip:axis_dwidth_converter:1.1 conv_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 8
} {
  S_AXIS scope_0/M_AXIS
  aclk /ps_0/FCLK_CLK0
  aresetn slice_1/Dout
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_1 {
  CONST_WIDTH 32
  CONST_VAL 503316480
}

# Create axis_ram_writer
cell pavel-demin:user:axis_ram_writer:1.0 writer_0 {
  ADDR_WIDTH 22
} {
  S_AXIS conv_0/M_AXIS
  M_AXI /ps_0/S_AXI_HP0
  cfg_data const_1/dout
  aclk /ps_0/FCLK_CLK0
  aresetn slice_1/Dout
}
