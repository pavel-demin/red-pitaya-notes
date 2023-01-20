set display_name {AXI4-Stream FIFO}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

core_parameter M_AXIS_TDATA_WIDTH {M_AXIS TDATA WIDTH} {Width of the M_AXIS data bus.}
core_parameter S_AXIS_TDATA_WIDTH {S_AXIS TDATA WIDTH} {Width of the S_AXIS data bus.}
core_parameter WRITE_DEPTH {WRITE DEPTH} {Depth of the write side of the FIFO buffer.}
core_parameter ALWAYS_READY {ALWAYS READY} {If TRUE, s_axis_tready is always high.}
core_parameter ALWAYS_VALID {ALWAYS VALID} {If TRUE, m_axis_tvalid is always high.}

set bus [ipx::get_bus_interfaces -of_objects $core m_axis]
set_property NAME M_AXIS $bus
set_property INTERFACE_MODE master $bus

set bus [ipx::get_bus_interfaces -of_objects $core s_axis]
set_property NAME S_AXIS $bus
set_property INTERFACE_MODE slave $bus

set bus [ipx::get_bus_interfaces aclk]
set parameter [ipx::get_bus_parameters -of_objects $bus ASSOCIATED_BUSIF]
set_property VALUE M_AXIS:S_AXIS $parameter
