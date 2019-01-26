# Create interface ports
create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_GP0

# Configure interface ports
set_property -dict [list CONFIG.ADDR_WIDTH 32 CONFIG.DATA_WIDTH 32 CONFIG.PROTOCOL AXI3] [get_bd_intf_ports M_AXI_GP0]

# Create ports
create_bd_port -dir O -type clk FCLK_CLK0
create_bd_port -dir O -type clk FCLK_CLK1
create_bd_port -dir O -type clk FCLK_CLK2
create_bd_port -dir O -type clk FCLK_CLK3
create_bd_port -dir O -type rst FCLK_RESET0_N
create_bd_port -dir O -type rst FCLK_RESET1_N
create_bd_port -dir O -type rst FCLK_RESET2_N
create_bd_port -dir O -type rst FCLK_RESET3_N
create_bd_port -dir I SPI0_MISO_I
create_bd_port -dir O SPI0_MISO_O
create_bd_port -dir O SPI0_MISO_T
create_bd_port -dir I SPI0_MOSI_I
create_bd_port -dir O SPI0_MOSI_O
create_bd_port -dir O SPI0_MOSI_T
create_bd_port -dir I SPI0_SCLK_I
create_bd_port -dir O SPI0_SCLK_O
create_bd_port -dir O SPI0_SCLK_T
create_bd_port -dir O SPI0_SS1_O
create_bd_port -dir O SPI0_SS2_O
create_bd_port -dir I SPI0_SS_I
create_bd_port -dir O SPI0_SS_O
create_bd_port -dir O SPI0_SS_T

# Configure ports
set_property CONFIG.ASSOCIATED_BUSIF M_AXI_GP0 [get_bd_ports FCLK_CLK0]

# Create processing_system7
cell xilinx.com:ip:processing_system7 ps_0 {
  PCW_IMPORT_BOARD_PRESET cfg/red_pitaya.xml
  PCW_EN_CLK1_PORT 1
  PCW_EN_CLK2_PORT 1
  PCW_EN_CLK3_PORT 1
  PCW_EN_RST1_PORT 1
  PCW_EN_RST2_PORT 1
  PCW_EN_RST3_PORT 1
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {
  make_external {FIXED_IO, DDR}
  Master Disable
  Slave Disable
} [get_bd_cells ps_0]

# Create interface connections
connect_bd_intf_net [get_bd_intf_ports M_AXI_GP0] [get_bd_intf_pins ps_0/M_AXI_GP0]

# Create port connections
connect_bd_net [get_bd_ports SPI0_MISO_I] [get_bd_pins ps_0/SPI0_MISO_I]
connect_bd_net [get_bd_ports SPI0_MOSI_I] [get_bd_pins ps_0/SPI0_MOSI_I]
connect_bd_net [get_bd_ports SPI0_SCLK_I] [get_bd_pins ps_0/SPI0_SCLK_I]
connect_bd_net [get_bd_ports SPI0_SS_I] [get_bd_pins ps_0/SPI0_SS_I]
connect_bd_net [get_bd_ports FCLK_CLK0] [get_bd_pins ps_0/FCLK_CLK0] [get_bd_pins ps_0/M_AXI_GP0_ACLK]
connect_bd_net [get_bd_ports FCLK_CLK1] [get_bd_pins ps_0/FCLK_CLK1]
connect_bd_net [get_bd_ports FCLK_CLK2] [get_bd_pins ps_0/FCLK_CLK2]
connect_bd_net [get_bd_ports FCLK_CLK3] [get_bd_pins ps_0/FCLK_CLK3]
connect_bd_net [get_bd_ports FCLK_RESET0_N] [get_bd_pins ps_0/FCLK_RESET0_N]
connect_bd_net [get_bd_ports FCLK_RESET1_N] [get_bd_pins ps_0/FCLK_RESET1_N]
connect_bd_net [get_bd_ports FCLK_RESET2_N] [get_bd_pins ps_0/FCLK_RESET2_N]
connect_bd_net [get_bd_ports FCLK_RESET3_N] [get_bd_pins ps_0/FCLK_RESET3_N]
connect_bd_net [get_bd_ports SPI0_MISO_O] [get_bd_pins ps_0/SPI0_MISO_O]
connect_bd_net [get_bd_ports SPI0_MISO_T] [get_bd_pins ps_0/SPI0_MISO_T]
connect_bd_net [get_bd_ports SPI0_MOSI_O] [get_bd_pins ps_0/SPI0_MOSI_O]
connect_bd_net [get_bd_ports SPI0_MOSI_T] [get_bd_pins ps_0/SPI0_MOSI_T]
connect_bd_net [get_bd_ports SPI0_SCLK_O] [get_bd_pins ps_0/SPI0_SCLK_O]
connect_bd_net [get_bd_ports SPI0_SCLK_T] [get_bd_pins ps_0/SPI0_SCLK_T]
connect_bd_net [get_bd_ports SPI0_SS1_O] [get_bd_pins ps_0/SPI0_SS1_O]
connect_bd_net [get_bd_ports SPI0_SS2_O] [get_bd_pins ps_0/SPI0_SS2_O]
connect_bd_net [get_bd_ports SPI0_SS_O] [get_bd_pins ps_0/SPI0_SS_O]
connect_bd_net [get_bd_ports SPI0_SS_T] [get_bd_pins ps_0/SPI0_SS_T]

# Create address segments
create_bd_addr_seg -range 0x40000000 -offset 0x40000000 [get_bd_addr_spaces ps_0/Data] [get_bd_addr_segs M_AXI_GP0/Reg] SEG_M_AXI_GP0_Reg
