source cfg_test/block_design.tcl

# Create axis_ram_writer
cell pavel-demin:user:axis_ram_writer:1.0 writer_0 {} {
  M_AXI ps_0/S_AXI_HP0
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn 
}