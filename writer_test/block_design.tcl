source cfg_test/block_design.tcl

# Create axis_ram_writer
cell pavel-demin:user:axis_ram_writer:1.0 writer_0 {} {
  M_AXI ps_0/S_AXI_HP0
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn 
}

create_bd_addr_seg -range 8M -offset 0x1E000000 [get_bd_addr_spaces writer_0/M_AXI] [get_bd_addr_segs ps_0/S_AXI_HP0/HP0_DDR_LOWOCM] SEG_ps_0_HP0_DDR_LOWOCM
