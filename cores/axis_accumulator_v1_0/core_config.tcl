set display_name {AXI4-Stream Accumulator}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

core_parameter S_AXIS_TDATA_WIDTH {S_AXIS TDATA WIDTH} {Width of the S_AXIS data buses.}
core_parameter M_AXIS_TDATA_WIDTH {M_AXIS TDATA WIDTH} {Width of the M_AXIS data buses.}
core_parameter CNTR_WIDTH {CNTR WIDTH} {Width of the counter register.}
core_parameter AXIS_TDATA_SIGNED {AXIS_TDATA_SIGNED} {If TRUE, the S_AXIS data are signed values.}
core_parameter CONTINUOUS {CONTINUOUS} {If TRUE, accumulator runs continuously.}

set bus [ipx::get_bus_interfaces -of_objects $core m_axis]
set_property NAME M_AXIS $bus
set_property INTERFACE_MODE master $bus

set bus [ipx::get_bus_interfaces -of_objects $core s_axis]
set_property NAME S_AXIS $bus
set_property INTERFACE_MODE slave $bus

set bus [ipx::get_bus_interfaces aclk]
set parameter [ipx::get_bus_parameters -of_objects $bus ASSOCIATED_BUSIF]
set_property VALUE M_AXIS:S_AXIS $parameter
