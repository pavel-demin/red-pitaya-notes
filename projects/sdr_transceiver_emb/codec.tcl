# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0 DOUT_WIDTH 1
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_1 {
  DIN_WIDTH 8 DIN_FROM 1 DIN_TO 1 DOUT_WIDTH 1
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_2 {
  DIN_WIDTH 8 DIN_FROM 2 DIN_TO 2 DOUT_WIDTH 1
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_3 {
  DIN_WIDTH 64 DIN_FROM 31 DIN_TO 0 DOUT_WIDTH 32
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_4 {
  DIN_WIDTH 64 DIN_FROM 47 DIN_TO 32 DOUT_WIDTH 16
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_5 {
  DIN_WIDTH 64 DIN_FROM 63 DIN_TO 48 DOUT_WIDTH 16
}

# Create axi_axis_writer
cell pavel-demin:user:axi_axis_writer:1.0 writer_0 {
  AXI_DATA_WIDTH 32
} {
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}

# Create fifo_generator
cell xilinx.com:ip:fifo_generator:13.1 fifo_generator_0 {
  PERFORMANCE_OPTIONS First_Word_Fall_Through
  INPUT_DATA_WIDTH 32
  INPUT_DEPTH 1024
  OUTPUT_DATA_WIDTH 32
  OUTPUT_DEPTH 1024
  DATA_COUNT true
  DATA_COUNT_WIDTH 11
} {
  clk /ps_0/FCLK_CLK0
  srst slice_0/Dout
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo:1.0 fifo_0 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
} {
  S_AXIS writer_0/M_AXIS
  FIFO_READ fifo_generator_0/FIFO_READ
  FIFO_WRITE fifo_generator_0/FIFO_WRITE
  aclk /ps_0/FCLK_CLK0
}

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen:8.3 bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 1024
  WRITE_WIDTH_B 32
  ENABLE_A Always_Enabled
  ENABLE_B Always_Enabled
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
}

# Create axi_bram_writer
cell pavel-demin:user:axi_bram_writer:1.0 writer_1 {
  AXI_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 10
} {
  BRAM_PORTA bram_0/BRAM_PORTA
}

# Create axis_keyer
cell pavel-demin:user:axis_keyer:1.0 keyer_0 {
  AXIS_TDATA_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 10
} {
  BRAM_PORTA bram_0/BRAM_PORTB
  cfg_data slice_4/Dout
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter:1.1 subset_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 4
  TDATA_REMAP {16'b0000000000000000,tdata[15:0]}
} {
  S_AXIS keyer_0/M_AXIS
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_constant
cell pavel-demin:user:axis_constant:1.0 phase_0 {
  AXIS_TDATA_WIDTH 32
} {
  cfg_data slice_3/Dout
  aclk /ps_0/FCLK_CLK0
}

# Create dds_compiler
cell xilinx.com:ip:dds_compiler:6.0 dds_0 {
  DDS_CLOCK_RATE 125
  SPURIOUS_FREE_DYNAMIC_RANGE 96
  FREQUENCY_RESOLUTION 0.2
  PHASE_INCREMENT Streaming
  HAS_TREADY true
  HAS_PHASE_OUT false
  PHASE_WIDTH 30
  OUTPUT_WIDTH 16
  DSP48_USE Minimal
} {
  S_AXIS_PHASE phase_0/M_AXIS
  aclk /ps_0/FCLK_CLK0
}

# Create axis_lfsr
cell pavel-demin:user:axis_lfsr:1.0 lfsr_0 {} {
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}

# Create cmpy
cell xilinx.com:ip:cmpy:6.0 mult_0 {
  FLOWCONTROL Blocking
  APORTWIDTH.VALUE_SRC USER
  BPORTWIDTH.VALUE_SRC USER
  APORTWIDTH 16
  BPORTWIDTH 16
  ROUNDMODE Random_Rounding
  OUTPUTWIDTH 18
} {
  S_AXIS_A subset_0/M_AXIS
  S_AXIS_B dds_0/M_AXIS_DATA
  S_AXIS_CTRL lfsr_0/M_AXIS
  aclk /ps_0/FCLK_CLK0
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter:1.1 subset_1 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 3
  M_TDATA_NUM_BYTES 4
  TDATA_REMAP {16'b0000000000000000,tdata[15:0]}
} {
  S_AXIS mult_0/M_AXIS_DOUT
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_constant
cell pavel-demin:user:axis_constant:1.0 const_0 {
  AXIS_TDATA_WIDTH 16
} {
  cfg_data slice_5/Dout
  aclk /ps_0/FCLK_CLK0
}

# Create axis_lfsr
cell pavel-demin:user:axis_lfsr:1.0 lfsr_1 {} {
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}

# Create cmpy
cell xilinx.com:ip:cmpy:6.0 mult_1 {
  FLOWCONTROL Blocking
  APORTWIDTH.VALUE_SRC USER
  BPORTWIDTH.VALUE_SRC USER
  APORTWIDTH 16
  BPORTWIDTH 16
  ROUNDMODE Random_Rounding
  OUTPUTWIDTH 18
} {
  S_AXIS_A subset_1/M_AXIS
  S_AXIS_B const_0/M_AXIS
  S_AXIS_CTRL lfsr_1/M_AXIS
  aclk /ps_0/FCLK_CLK0
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter:1.1 subset_2 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 3
  M_TDATA_NUM_BYTES 4
  TDATA_REMAP {tdata[15:0],tdata[15:0]}
} {
  S_AXIS mult_1/M_AXIS_DOUT
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_switch
cell xilinx.com:ip:axis_switch:1.1 switch_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
  ROUTING_MODE 1
  NUM_SI 2
  NUM_MI 1
} {
  S00_AXIS fifo_0/M_AXIS
  S01_AXIS subset_2/M_AXIS
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_i2s
cell pavel-demin:user:axis_i2s:1.0 i2s_0 {
  AXIS_TDATA_WIDTH 32
} {
  alex_flag slice_2/Dout
  S_AXIS switch_0/M00_AXIS
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}

# Create fifo_generator
cell xilinx.com:ip:fifo_generator:13.1 fifo_generator_1 {
  PERFORMANCE_OPTIONS First_Word_Fall_Through
  INPUT_DATA_WIDTH 32
  INPUT_DEPTH 1024
  OUTPUT_DATA_WIDTH 32
  OUTPUT_DEPTH 1024
  DATA_COUNT true
  DATA_COUNT_WIDTH 11
} {
  clk /ps_0/FCLK_CLK0
  srst slice_1/Dout
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo:1.0 fifo_1 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
} {
  S_AXIS i2s_0/M_AXIS
  FIFO_READ fifo_generator_1/FIFO_READ
  FIFO_WRITE fifo_generator_1/FIFO_WRITE
  aclk /ps_0/FCLK_CLK0
}

# Create axi_axis_reader
cell pavel-demin:user:axi_axis_reader:1.0 reader_0 {
  AXI_DATA_WIDTH 32
} {
  S_AXIS fifo_1/M_AXIS
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}
