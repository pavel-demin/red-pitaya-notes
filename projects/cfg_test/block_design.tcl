source projects/base_system/block_design.tcl

# Create xlconstant
cell xilinx.com:ip:xlconstant const_0

# Create proc_sys_reset
cell xilinx.com:ip:proc_sys_reset rst_0 {} {
  ext_reset_in const_0/dout
}

# Create c_counter_binary
cell xilinx.com:ip:c_counter_binary cntr_0 {
  Output_Width 32
} {
  CLK pll_0/clk_out1
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_FROM 26
  DIN_TO 26
} {
  din cntr_0/Q
}

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register cfg_0 {
  CFG_DATA_WIDTH 1024
  AXI_ADDR_WIDTH 7
  AXI_DATA_WIDTH 32
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins cfg_0/S_AXI]

assign_bd_address -range 4K -offset 0x40000000 [get_bd_addr_segs -of_objects [get_bd_intf_pins cfg_0/S_AXI]]

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 1024 DIN_FROM 134 DIN_TO 128
} {
  din cfg_0/cfg_data
}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  IN1_WIDTH 7
} {
  In0 slice_0/dout
  In1 slice_1/dout
  dout led_o
}
