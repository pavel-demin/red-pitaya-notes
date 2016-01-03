# timer_0/aresetn

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0 DOUT_WIDTH 1
}

# pha_0/aresetn and hist_0/aresetn

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_1 {
  DIN_WIDTH 8 DIN_FROM 1 DIN_TO 1 DOUT_WIDTH 1
}

# pha_0/bln_flag

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_2 {
  DIN_WIDTH 8 DIN_FROM 2 DIN_TO 2 DOUT_WIDTH 1
}

# timer_0/run_flag

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_3 {
  DIN_WIDTH 8 DIN_FROM 3 DIN_TO 3 DOUT_WIDTH 1
}

# timer_0/cfg_flag

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_4 {
  DIN_WIDTH 8 DIN_FROM 4 DIN_TO 4 DOUT_WIDTH 1
}

# timer_0/cfg_data

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_5 {
  DIN_WIDTH 128 DIN_FROM 63 DIN_TO 0 DOUT_WIDTH 64
}

# pha_0/cfg_data

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_6 {
  DIN_WIDTH 128 DIN_FROM 79 DIN_TO 64 DOUT_WIDTH 16
}

# pha_0/min_data

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_7 {
  DIN_WIDTH 128 DIN_FROM 95 DIN_TO 80 DOUT_WIDTH 16
}

# pha_0/max_data

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_8 {
  DIN_WIDTH 128 DIN_FROM 111 DIN_TO 96 DOUT_WIDTH 16
}

# Create axis_validator
cell pavel-demin:user:axis_timer:1.0 timer_0 {
  CNTR_WIDTH 64
} {
  run_flag slice_3/Dout
  cfg_flag slice_4/Dout
  cfg_data slice_5/Dout
  aclk /ps_0/FCLK_CLK0
  aresetn slice_0/Dout
}
# Create axis_pulse_height_analyzer
cell pavel-demin:user:axis_pulse_height_analyzer:1.0 pha_0 {
  AXIS_TDATA_WIDTH 16
  AXIS_TDATA_SIGNED TRUE
  CNTR_WIDTH 16
} {
  bln_flag slice_2/Dout
  cfg_data slice_6/Dout
  min_data slice_7/Dout
  max_data slice_8/Dout
  aclk /ps_0/FCLK_CLK0
  aresetn slice_1/Dout
}

# Create axis_validator
cell pavel-demin:user:axis_validator:1.0 vldtr_0 {
  AXIS_TDATA_WIDTH 16
} {
  S_AXIS pha_0/M_AXIS
  trg_flag timer_0/trg_flag
  aclk /ps_0/FCLK_CLK0
}

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen:8.3 bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  USE_BYTE_WRITE_ENABLE true
  BYTE_SIZE 8
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 16384
  WRITE_WIDTH_B 32
  WRITE_DEPTH_B 16384
  ENABLE_A Always_Enabled
  ENABLE_B Always_Enabled
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
}

# Create axis_histogram
cell pavel-demin:user:axis_histogram:1.0 hist_0 {
  BRAM_ADDR_WIDTH 14
  BRAM_DATA_WIDTH 32
  AXIS_TDATA_WIDTH 16
} {
  S_AXIS vldtr_0/M_AXIS
  BRAM_PORTA bram_0/BRAM_PORTA
  aclk /ps_0/FCLK_CLK0
  aresetn slice_1/Dout
}

# Create axi_bram_reader
cell pavel-demin:user:axi_bram_reader:1.0 reader_0 {
  AXI_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 14
} {
  BRAM_PORTA bram_0/BRAM_PORTB
}
