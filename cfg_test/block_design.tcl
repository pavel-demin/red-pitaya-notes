source base_system/block_design.tcl

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register:1.0 cfg_0 {
  CFG_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins cfg_0/s_axi_lite]

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_0 {
  DIN_FROM 0
  DIN_TO 7
} {
  Din cfg_0/cfg_data
  Dout led_o
}
