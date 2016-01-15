set display_name {AXI4-Stream FIFO}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

core_parameter M_AXIS_TDATA_WIDTH {M_AXIS TDATA WIDTH} {Width of the M_AXIS data bus.}
core_parameter S_AXIS_TDATA_WIDTH {S_AXIS TDATA WIDTH} {Width of the S_AXIS data bus.}

set bus [ipx::get_bus_interfaces -of_objects $core m_axis]
set_property NAME M_AXIS $bus
set_property INTERFACE_MODE master $bus

set bus [ipx::get_bus_interfaces -of_objects $core s_axis]
set_property NAME S_AXIS $bus
set_property INTERFACE_MODE slave $bus

set bus [ipx::add_bus_interface FIFO_READ $core]
set_property ABSTRACTION_TYPE_VLNV xilinx.com:interface:fifo_read_rtl:1.0 $bus
set_property BUS_TYPE_VLNV xilinx.com:interface:fifo_read:1.0 $bus
set_property INTERFACE_MODE master $bus
foreach {logical physical} {
  EMPTY   fifo_read_empty
  RD_DATA fifo_read_data
  RD_EN   fifo_read_rden
} {
  set_property PHYSICAL_NAME $physical [ipx::add_port_map $logical $bus]
}

set bus [ipx::add_bus_interface FIFO_WRITE $core]
set_property ABSTRACTION_TYPE_VLNV xilinx.com:interface:fifo_write_rtl:1.0 $bus
set_property BUS_TYPE_VLNV xilinx.com:interface:fifo_write:1.0 $bus
set_property INTERFACE_MODE master $bus
foreach {logical physical} {
  FULL    fifo_write_full
  WR_DATA fifo_write_data
  WR_EN   fifo_write_wren
} {
  set_property PHYSICAL_NAME $physical [ipx::add_port_map $logical $bus]
}

set bus [ipx::get_bus_interfaces aclk]
set parameter [ipx::get_bus_parameters -of_objects $bus ASSOCIATED_BUSIF]
set_property VALUE M_AXIS:S_AXIS:FIFO_READ:FIFO_WRITE $parameter
