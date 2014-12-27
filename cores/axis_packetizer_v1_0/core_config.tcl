set display_name {AXI4-Stream Packetizer}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

core_parameter AXIS_TDATA_WIDTH {AXIS TDATA WIDTH} {Width of the M_AXIS and S_AXIS data buses.}
core_parameter PACKET_SIZE {PACKET SIZE} {Size of the packet.}

set bus [ipx::get_bus_interfaces -of_objects $core m_axis]
set_property NAME M_AXIS $bus
set_property INTERFACE_MODE master $bus

set bus [ipx::get_bus_interfaces -of_objects $core s_axis]
set_property NAME S_AXIS $bus
set_property INTERFACE_MODE slave $bus

set bus [ipx::get_bus_interfaces signal_clock]
set parameter [ipx::get_bus_parameters -of_objects $bus ASSOCIATED_BUSIF]
set_property VALUE M_AXIS:S_AXIS $parameter
