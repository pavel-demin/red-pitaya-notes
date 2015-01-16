set display_name {AXI4-Stream Histogram}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

core_parameter S_AXIS_TDATA_WIDTH {S_AXIS TDATA WIDTH} {Width of the S_AXIS data bus.}
core_parameter M_AXIS_TDATA_WIDTH {M_AXIS TDATA WIDTH} {Width of the M_AXIS data bus.}
core_parameter BRAM_ADDR_WIDTH {BRAM ADDR WIDTH} {Width of the BRAM address ports.}

set bus [ipx::get_bus_interfaces -of_objects $core m_axis]
set_property NAME M_AXIS $bus
set_property INTERFACE_MODE master $bus

set bus [ipx::get_bus_interfaces -of_objects $core s_axis]
set_property NAME S_AXIS $bus
set_property INTERFACE_MODE slave $bus

set bus [ipx::get_bus_interfaces signal_clock]
set parameter [ipx::get_bus_parameters -of_objects $bus ASSOCIATED_BUSIF]
set_property VALUE M_AXIS:S_AXIS $parameter

set bus [ipx::add_bus_interface BRAM_PORTA $core]
set_property ABSTRACTION_TYPE_VLNV xilinx.com:interface:bram_rtl:1.0 $bus
set_property BUS_TYPE_VLNV xilinx.com:interface:bram:1.0 $bus
set_property INTERFACE_MODE master $bus
foreach {logical physical} {
  RST  bram_porta_rst
  CLK  bram_porta_clk
  ADDR bram_porta_addr
  DIN  bram_porta_wrdata
  DOUT bram_porta_rddata
  WE   bram_porta_we
} {
  set_property PHYSICAL_NAME $physical [ipx::add_port_map $logical $bus]
}

set bus [ipx::get_bus_interfaces bram_porta_signal_clock]
set parameter [ipx::add_bus_parameter ASSOCIATED_BUSIF $bus]
set_property VALUE BRAM_PORTA $parameter

set bus [ipx::add_bus_interface BRAM_PORTB $core]
set_property ABSTRACTION_TYPE_VLNV xilinx.com:interface:bram_rtl:1.0 $bus
set_property BUS_TYPE_VLNV xilinx.com:interface:bram:1.0 $bus
set_property INTERFACE_MODE master $bus
foreach {logical physical} {
  RST  bram_portb_rst
  CLK  bram_portb_clk
  ADDR bram_portb_addr
  DIN  bram_portb_wrdata
  DOUT bram_portb_rddata
  WE   bram_portb_we
} {
  set_property PHYSICAL_NAME $physical [ipx::add_port_map $logical $bus]
}

set bus [ipx::get_bus_interfaces bram_portb_signal_clock]
set parameter [ipx::add_bus_parameter ASSOCIATED_BUSIF $bus]
set_property VALUE BRAM_PORTB $parameter
