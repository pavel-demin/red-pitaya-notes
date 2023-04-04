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

set hub_size 6

for {set i 0} {$i < $hub_size} {incr i} {
  set index [format %02d $i]

  set bus [ipx::add_bus_interface B${index}_BRAM $core]
  set_property ABSTRACTION_TYPE_VLNV xilinx.com:interface:bram_rtl:1.0 $bus
  set_property BUS_TYPE_VLNV xilinx.com:interface:bram:1.0 $bus
  set_property INTERFACE_MODE master $bus
  foreach {logical physical} {
    RST  rst
    CLK  clk
    EN   en
    WE   we
    ADDR addr
    DIN  wdata
    DOUT rdata
  } {
    set_property PHYSICAL_NAME b${index}_bram_$physical [ipx::add_port_map $logical $bus]
  }

  set bus [ipx::get_bus_interfaces b${index}_bram_clk]
  set parameter [ipx::add_bus_parameter ASSOCIATED_BUSIF $bus]
  set_property VALUE B${index}_BRAM $parameter

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
