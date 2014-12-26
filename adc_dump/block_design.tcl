source led_blinker/block_design.tcl

# Enable HP0
set_property -dict [list CONFIG.PCW_USE_S_AXI_HP0 {1}] [get_bd_cells ps_0]

# Enable interrupts
set_property -dict [list CONFIG.PCW_USE_FABRIC_INTERRUPT {1}] [get_bd_cells ps_0]
set_property -dict [list CONFIG.PCW_IRQ_F2P_INTR {1}] [get_bd_cells ps_0]

# Create axis_clock_converter
create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 fifo_0

# Connect axis_red_pitaya_adc to axis_clock_converter
connect_bd_net [get_bd_pins adc_0/adc_clk] [get_bd_pins fifo_0/s_axis_aclk]
connect_bd_intf_net [get_bd_intf_pins adc_0/M_AXIS] [get_bd_intf_pins fifo_0/S_AXIS]

# Create axis_packetizer
create_bd_cell -type ip -vlnv pavel-demin:user:axis_packetizer:1.0 pktzr_0

# Connect axis_clock_converter to axis_packetizer
connect_bd_intf_net [get_bd_intf_pins fifo_0/M_AXIS] [get_bd_intf_pins pktzr_0/S_AXIS]

# Create axi_dma
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dma:7.1 dma_0

# Connect axis_packetizer to axi_dma
connect_bd_intf_net [get_bd_intf_pins pktzr_0/M_AXIS] [get_bd_intf_pins dma_0/S_AXIS_S2MM]

# Create axi_interconnect for GP0
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 gp_0
set_property -dict [list CONFIG.NUM_MI {1}] [get_bd_cells gp_0]

# Connect processing_system7 to axi_interconnect
connect_bd_intf_net [get_bd_intf_pins ps_0/M_AXI_GP0] [get_bd_intf_pins gp_0/S00_AXI]

# Connect axi_interconnect to axi_dma
connect_bd_intf_net [get_bd_intf_pins gp_0/M00_AXI] [get_bd_intf_pins dma_0/S_AXI_LITE]

# Create axi_interconnect for HP0
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 hp_0
set_property -dict [list CONFIG.NUM_MI {1} CONFIG.NUM_SI {3}]  [get_bd_cells hp_0]

# Connect axi_dma to axi_interconnect
connect_bd_intf_net [get_bd_intf_pins dma_0/M_AXI_MM2S] [get_bd_intf_pins hp_0/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins dma_0/M_AXI_S2MM] [get_bd_intf_pins hp_0/S02_AXI]
connect_bd_intf_net [get_bd_intf_pins dma_0/M_AXI_SG] [get_bd_intf_pins hp_0/S00_AXI]

# Connect axi_interconnect to processing_system7
connect_bd_intf_net [get_bd_intf_pins hp_0/M00_AXI] [get_bd_intf_pins ps_0/S_AXI_HP0]

# Create xlconcat
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 concat_0

# Connect axi_dma to xlconcat
connect_bd_net [get_bd_pins dma_0/mm2s_introut] [get_bd_pins concat_0/In0]
connect_bd_net [get_bd_pins dma_0/s2mm_introut] [get_bd_pins concat_0/In1]

# Connect xlconcat to processing_system7
connect_bd_net [get_bd_pins concat_0/dout] [get_bd_pins ps_0/IRQ_F2P]

# Create proc_sys_reset
create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 rst_0

# Connect processing_system7 to proc_sys_reset
connect_bd_net [get_bd_pins ps_0/FCLK_RESET0_N] [get_bd_pins rst_0/ext_reset_in]

# Connect proc_sys_reset to all other cells
connect_bd_net [get_bd_pins rst_0/interconnect_aresetn] [get_bd_pins gp_0/ARESETN] [get_bd_pins hp_0/ARESETN]
connect_bd_net [get_bd_pins rst_0/peripheral_aresetn] [get_bd_pins dma_0/axi_resetn] [get_bd_pins fifo_0/m_axis_aresetn] [get_bd_pins gp_0/M00_ARESETN] [get_bd_pins gp_0/S00_ARESETN] [get_bd_pins hp_0/M00_ARESETN] [get_bd_pins hp_0/S00_ARESETN] [get_bd_pins hp_0/S01_ARESETN] [get_bd_pins hp_0/S02_ARESETN] [get_bd_pins pktzr_0/aresetn]

# Connect FCLK_CLK0 to all other cells
connect_bd_net [get_bd_pins ps_0/FCLK_CLK0] [get_bd_pins dma_0/m_axi_mm2s_aclk] [get_bd_pins dma_0/m_axi_s2mm_aclk] [get_bd_pins dma_0/m_axi_sg_aclk] [get_bd_pins dma_0/s_axi_lite_aclk] [get_bd_pins fifo_0/m_axis_aclk] [get_bd_pins gp_0/ACLK] [get_bd_pins gp_0/M00_ACLK] [get_bd_pins gp_0/S00_ACLK] [get_bd_pins hp_0/ACLK] [get_bd_pins hp_0/M00_ACLK] [get_bd_pins hp_0/S00_ACLK] [get_bd_pins hp_0/S01_ACLK] [get_bd_pins hp_0/S02_ACLK] [get_bd_pins pktzr_0/aclk] [get_bd_pins ps_0/S_AXI_HP0_ACLK] [get_bd_pins rst_0/slowest_sync_clk]

# Configure addresses
create_bd_addr_seg -range 0x20000000 -offset 0x0 [get_bd_addr_spaces dma_0/Data_SG] [get_bd_addr_segs ps_0/S_AXI_HP0/HP0_DDR_LOWOCM] SEG_ps_0_HP0_DDR_LOWOCM
create_bd_addr_seg -range 0x20000000 -offset 0x0 [get_bd_addr_spaces dma_0/Data_MM2S] [get_bd_addr_segs ps_0/S_AXI_HP0/HP0_DDR_LOWOCM] SEG_ps_0_HP0_DDR_LOWOCM
create_bd_addr_seg -range 0x20000000 -offset 0x0 [get_bd_addr_spaces dma_0/Data_S2MM] [get_bd_addr_segs ps_0/S_AXI_HP0/HP0_DDR_LOWOCM] SEG_ps_0_HP0_DDR_LOWOCM
create_bd_addr_seg -range 0x10000 -offset 0x40400000 [get_bd_addr_spaces ps_0/Data] [get_bd_addr_segs dma_0/S_AXI_LITE/Reg] SEG_axi_dma_0_Reg
