
# Create processing_system7
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps_0

# Import Red Pitaya configuration
set_property -dict [list CONFIG.PCW_IMPORT_BOARD_PRESET {cfg/red_pitaya.xml}] [get_bd_cells ps_0]

# Connect FCLK_CLK0 to M_AXI_GP0_ACLK
connect_bd_net [get_bd_pins ps_0/FCLK_CLK0] [get_bd_pins ps_0/M_AXI_GP0_ACLK]

# Create interface ports for processing_system7
create_bd_intf_port -mode Master -vlnv xilinx.com:interface:ddrx_rtl:1.0 DDR
create_bd_intf_port -mode Master -vlnv xilinx.com:display_processing_system7:fixedio_rtl:1.0 FIXED_IO

# Connect processing_system7 to interface ports
connect_bd_intf_net [get_bd_intf_ports DDR] [get_bd_intf_pins ps_0/DDR]
connect_bd_intf_net [get_bd_intf_ports FIXED_IO] [get_bd_intf_pins ps_0/FIXED_IO]
