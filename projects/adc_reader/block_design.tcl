source projects/led_blinker/block_design.tcl

# Create proc_sys_reset
cell xilinx.com:ip:proc_sys_reset:5.0 rst_0

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_1 {} {
  dout rst_0/aux_reset_in
}

# Create axi_dma
cell xilinx.com:ip:axi_dma:7.1 dma_0

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins dma_0/S_AXI_LITE]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /dma_0/M_AXI_SG
  Clk Auto
} [get_bd_intf_pins ps_0/S_AXI_HP0]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Slave /ps_0/S_AXI_HP0
  Clk Auto
} [get_bd_intf_pins dma_0/M_AXI_MM2S]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Slave /ps_0/S_AXI_HP0
  Clk Auto
} [get_bd_intf_pins dma_0/M_AXI_S2MM]

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_1 {} {
  In0 dma_0/mm2s_introut
  In1 dma_0/s2mm_introut
  dout ps_0/IRQ_F2P
}

# Create axis_packetizer
cell pavel-demin:user:axis_packetizer:1.0 pktzr_0 {} {
  M_AXIS dma_0/S_AXIS_S2MM
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_clock_converter
cell xilinx.com:ip:axis_clock_converter:1.1 fifo_0 {} {
  M_AXIS pktzr_0/S_AXIS
  m_axis_aclk ps_0/FCLK_CLK0
  m_axis_aresetn rst_0/peripheral_aresetn
  S_AXIS adc_0/M_AXIS
  s_axis_aclk adc_0/adc_clk
}

# Create xadc_wiz
cell xilinx.com:ip:xadc_wiz:3.0 xadc_0 {
  XADC_STARUP_SELECTION channel_sequencer
  SEQUENCER_MODE Continuous
  CHANNEL_ENABLE_TEMPERATURE true
  CHANNEL_ENABLE_VCCINT true
  CHANNEL_ENABLE_VCCAUX true
  CHANNEL_ENABLE_VREFP false
  CHANNEL_ENABLE_VBRAM true
  CHANNEL_ENABLE_VCCPINT true
  CHANNEL_ENABLE_VCCPAUX true
  CHANNEL_ENABLE_VCCDDRO true
  CHANNEL_ENABLE_VP_VN true
  CHANNEL_ENABLE_VAUXP0_VAUXN0 true
  CHANNEL_ENABLE_VAUXP1_VAUXN1 true
  CHANNEL_ENABLE_VAUXP8_VAUXN8 true
  CHANNEL_ENABLE_VAUXP9_VAUXN9 true
  BIPOLAR_VP_VN true
  BIPOLAR_VAUXP0_VAUXN0 true
  BIPOLAR_VAUXP1_VAUXN1 true
  BIPOLAR_VAUXP8_VAUXN8 true
  BIPOLAR_VAUXP9_VAUXN9 true
  USER_TEMP_ALARM false
  VCCINT_ALARM false
  VCCAUX_ALARM false
  OT_ALARM false
  ENABLE_VCCPINT_ALARM false
  ENABLE_VCCPAUX_ALARM false
  ENABLE_VCCDDRO_ALARM false
} {
  Vp_Vn Vp_Vn
  Vaux0 Vaux0
  Vaux1 Vaux1
  Vaux8 Vaux8
  Vaux9 Vaux9
  s_axi_aresetn rst_0/peripheral_aresetn
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins xadc_0/s_axi_lite]
