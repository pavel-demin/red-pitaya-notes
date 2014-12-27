source led_blinker/block_design.tcl

# Enable HP0
set_property CONFIG.PCW_USE_S_AXI_HP0 1 [get_bd_cells ps_0]

# Enable interrupts
set_property CONFIG.PCW_USE_FABRIC_INTERRUPT 1 [get_bd_cells ps_0]
set_property CONFIG.PCW_IRQ_F2P_INTR 1 [get_bd_cells ps_0]

# Create proc_sys_reset
create_bd_cell -vlnv xilinx.com:ip:proc_sys_reset:5.0 rst_0

# Create xlconstant
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 const_0

# Connect xlconstant to proc_sys_reset
connect_bd_net [get_bd_pins const_0/dout] [get_bd_pins rst_0/aux_reset_in]

# Create axi_dma
create_bd_cell -vlnv xilinx.com:ip:axi_dma:7.1 dma_0

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master /ps_0/M_AXI_GP0 Clk Auto} [get_bd_intf_pins dma_0/S_AXI_LITE]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master /dma_0/M_AXI_SG Clk Auto} [get_bd_intf_pins ps_0/S_AXI_HP0]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Slave /ps_0/S_AXI_HP0 Clk Auto} [get_bd_intf_pins dma_0/M_AXI_MM2S]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Slave /ps_0/S_AXI_HP0 Clk Auto} [get_bd_intf_pins dma_0/M_AXI_S2MM]

# Create xlconcat
create_bd_cell -vlnv xilinx.com:ip:xlconcat:2.1 concat_0

# Connect axi_dma to xlconcat
connect_bd_net [get_bd_pins dma_0/mm2s_introut] [get_bd_pins concat_0/In0]
connect_bd_net [get_bd_pins dma_0/s2mm_introut] [get_bd_pins concat_0/In1]

# Connect xlconcat to processing_system7
connect_bd_net [get_bd_pins concat_0/dout] [get_bd_pins ps_0/IRQ_F2P]

# Create axis_packetizer
create_bd_cell -vlnv pavel-demin:user:axis_packetizer:1.0 pktzr_0

# Connect axis_packetizer to axi_dma
connect_bd_intf_net [get_bd_intf_pins pktzr_0/M_AXIS] [get_bd_intf_pins dma_0/S_AXIS_S2MM]

# Connect axis_packetizer to clock and reset nets
connect_bd_net [get_bd_pins pktzr_0/aclk] [get_bd_pins ps_0/FCLK_CLK0]
connect_bd_net [get_bd_pins pktzr_0/aresetn] [get_bd_pins rst_0/peripheral_aresetn]

# Create axis_clock_converter
create_bd_cell -vlnv xilinx.com:ip:axis_clock_converter:1.1 fifo_0

# Connect axis_clock_converter to axis_packetizer
connect_bd_intf_net [get_bd_intf_pins fifo_0/M_AXIS] [get_bd_intf_pins pktzr_0/S_AXIS]

# Connect axis_clock_converter to axis_red_pitaya_adc
connect_bd_intf_net [get_bd_intf_pins fifo_0/S_AXIS] [get_bd_intf_pins adc_0/M_AXIS]

# Connect axis_clock_converter to clock and reset nets
connect_bd_net [get_bd_pins fifo_0/m_axis_aclk] [get_bd_pins ps_0/FCLK_CLK0]
connect_bd_net [get_bd_pins fifo_0/m_axis_aresetn] [get_bd_pins rst_0/peripheral_aresetn]
connect_bd_net [get_bd_pins fifo_0/s_axis_aclk] [get_bd_pins adc_0/adc_clk]

# Create xadc_wiz
create_bd_cell -vlnv xilinx.com:ip:xadc_wiz:3.0 xadc_0

# Configure xadc_wiz
set_property -dict [list CONFIG.XADC_STARUP_SELECTION {channel_sequencer} CONFIG.CHANNEL_ENABLE_TEMPERATURE {true} CONFIG.CHANNEL_ENABLE_VCCINT {true} CONFIG.CHANNEL_ENABLE_VCCAUX {true} CONFIG.CHANNEL_ENABLE_VREFP {false} CONFIG.CHANNEL_ENABLE_VBRAM {true} CONFIG.CHANNEL_ENABLE_VCCPINT {true} CONFIG.CHANNEL_ENABLE_VCCPAUX {true} CONFIG.CHANNEL_ENABLE_VCCDDRO {true} CONFIG.CHANNEL_ENABLE_VAUXP0_VAUXN0 {true} CONFIG.CHANNEL_ENABLE_VAUXP1_VAUXN1 {true} CONFIG.CHANNEL_ENABLE_VAUXP3_VAUXN3 {false} CONFIG.CHANNEL_ENABLE_VAUXP8_VAUXN8 {true} CONFIG.CHANNEL_ENABLE_VAUXP9_VAUXN9 {true} CONFIG.BIPOLAR_VP_VN {true} CONFIG.BIPOLAR_VAUXP0_VAUXN0 {true} CONFIG.BIPOLAR_VAUXP1_VAUXN1 {true} CONFIG.BIPOLAR_VAUXP8_VAUXN8 {true} CONFIG.BIPOLAR_VAUXP9_VAUXN9 {true} CONFIG.SEQUENCER_MODE {Continuous} CONFIG.USER_TEMP_ALARM {false} CONFIG.VCCINT_ALARM {false} CONFIG.VCCAUX_ALARM {false} CONFIG.OT_ALARM {false} CONFIG.ENABLE_VCCPINT_ALARM {false} CONFIG.ENABLE_VCCPAUX_ALARM {false} CONFIG.ENABLE_VCCDDRO_ALARM {false} CONFIG.CHANNEL_ENABLE_VP_VN {true}] [get_bd_cells xadc_0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master /ps_0/M_AXI_GP0 Clk Auto} [get_bd_intf_pins xadc_0/s_axi_lite]

# Connect xadc_wiz to reset net
connect_bd_net [get_bd_pins xadc_0/s_axi_aresetn] [get_bd_pins rst_0/peripheral_aresetn]

# Connect xadc_wiz to interface ports
connect_bd_intf_net [get_bd_intf_pins xadc_0/Vp_Vn] [get_bd_intf_ports Vp_Vn]
connect_bd_intf_net [get_bd_intf_pins xadc_0/Vaux0] [get_bd_intf_ports Vaux0]
connect_bd_intf_net [get_bd_intf_pins xadc_0/Vaux1] [get_bd_intf_ports Vaux1]
connect_bd_intf_net [get_bd_intf_pins xadc_0/Vaux8] [get_bd_intf_ports Vaux8]
connect_bd_intf_net [get_bd_intf_pins xadc_0/Vaux9] [get_bd_intf_ports Vaux9]
