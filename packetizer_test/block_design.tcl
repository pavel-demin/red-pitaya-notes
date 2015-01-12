source cfg_test/block_design.tcl

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_2 {
  DIN_WIDTH 1024 DIN_FROM 0 DIN_TO 0 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_3 {
  DIN_WIDTH 1024 DIN_FROM 1 DIN_TO 1 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_4 {
  DIN_WIDTH 1024 DIN_FROM 63 DIN_TO 32 DOUT_WIDTH 32
} {
  Din cfg_0/cfg_data
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_1

# Create axis_clock_converter
cell xilinx.com:ip:axis_clock_converter:1.1 fifo_0 {} {
  S_AXIS adc_0/M_AXIS
  s_axis_aclk adc_0/adc_clk
  s_axis_aresetn const_1/dout
  m_axis_aclk ps_0/FCLK_CLK0
  m_axis_aresetn rst_0/peripheral_aresetn
}

# Create axis_packetizer
cell pavel-demin:user:axis_packetizer:1.0 pktzr_0 {} {
  S_AXIS fifo_0/M_AXIS
  cfg_data slice_4/Dout
  aclk ps_0/FCLK_CLK0
  aresetn slice_2/Dout
}

# Create axis_ram_writer
cell pavel-demin:user:axis_ram_writer:1.0 writer_0 {} {
  S_AXIS pktzr_0/M_AXIS
  M_AXI ps_0/S_AXI_HP0
  aclk ps_0/FCLK_CLK0
  aresetn slice_3/Dout
}

create_bd_addr_seg -range 8M -offset 0x1E000000 [get_bd_addr_spaces writer_0/M_AXI] [get_bd_addr_segs ps_0/S_AXI_HP0/HP0_DDR_LOWOCM] SEG_ps_0_HP0_DDR_LOWOCM
