set display_name {AXI4 Hub}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

core_parameter CFG_DATA_WIDTH {CFG DATA WIDTH} {Width of the configuration data.}
core_parameter STS_DATA_WIDTH {STS DATA WIDTH} {Width of the status data.}

set bus [ipx::get_bus_interfaces -of_objects $core s_axi]
set_property NAME S_AXI $bus
set_property INTERFACE_MODE slave $bus

set buses [list S_AXI]

set num_axis 16

for {set i 0} {$i < $num_axis} {incr i} {
  set index [format %02d $i]

  set bus [ipx::get_bus_interfaces -of_objects $core s${index}_axis]
  set_property NAME S${index}_AXIS $bus
  set_property INTERFACE_MODE slave $bus

  set bus [ipx::get_bus_interfaces -of_objects $core m${index}_axis]
  set_property NAME M${index}_AXIS $bus
  set_property INTERFACE_MODE master $bus

  lappend buses S${index}_AXIS M${index}_AXIS
}

set bus [ipx::get_bus_interfaces aclk]
set parameter [ipx::get_bus_parameters -of_objects $bus ASSOCIATED_BUSIF]
set_property VALUE [join $buses :] $parameter

set bus [ipx::add_bus_interface BRAM_PORTA $core]
set_property ABSTRACTION_TYPE_VLNV xilinx.com:interface:bram_rtl:1.0 $bus
set_property BUS_TYPE_VLNV xilinx.com:interface:bram:1.0 $bus
set_property INTERFACE_MODE master $bus
foreach {logical physical} {
  RST  bram_porta_rst
  CLK  bram_porta_clk
  EN   bram_porta_en
  WE   bram_porta_we
  ADDR bram_porta_addr
  DIN  bram_porta_wrdata
  DOUT bram_porta_rddata
} {
  set_property PHYSICAL_NAME $physical [ipx::add_port_map $logical $bus]
}

set bus [ipx::get_bus_interfaces bram_porta_clk]
set parameter [ipx::add_bus_parameter ASSOCIATED_BUSIF $bus]
set_property VALUE BRAM_PORTA $parameter

set bus [ipx::add_bus_interface BRAM_PORTB $core]
set_property ABSTRACTION_TYPE_VLNV xilinx.com:interface:bram_rtl:1.0 $bus
set_property BUS_TYPE_VLNV xilinx.com:interface:bram:1.0 $bus
set_property INTERFACE_MODE master $bus
foreach {logical physical} {
  RST  bram_portb_rst
  CLK  bram_portb_clk
  EN   bram_portb_en
  WE   bram_portb_we
  ADDR bram_portb_addr
  DIN  bram_portb_wrdata
  DOUT bram_portb_rddata
} {
  set_property PHYSICAL_NAME $physical [ipx::add_port_map $logical $bus]
}

set bus [ipx::get_bus_interfaces bram_portb_clk]
set parameter [ipx::add_bus_parameter ASSOCIATED_BUSIF $bus]
set_property VALUE BRAM_PORTB $parameter
