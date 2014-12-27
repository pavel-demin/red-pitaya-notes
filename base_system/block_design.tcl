
# Create processing_system7
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps_0

# Import Red Pitaya configuration
set_property CONFIG.PCW_IMPORT_BOARD_PRESET cfg/red_pitaya.xml [get_bd_cells ps_0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {make_external "FIXED_IO, DDR" Master Disable Slave Disable} [get_bd_cells ps_0]

# Connect FCLK_CLK0 to M_AXI_GP0_ACLK
connect_bd_net [get_bd_pins ps_0/FCLK_CLK0] [get_bd_pins ps_0/M_AXI_GP0_ACLK]
