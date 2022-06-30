set display_name {AXI4-Stream RAM Writer}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

core_parameter ADDR_WIDTH {ADDR WIDTH} {Width of the address.}
core_parameter AXI_ID_WIDTH {AXI ID WIDTH} {Width of the AXI ID bus.}
core_parameter AXI_ADDR_WIDTH {AXI ADDR WIDTH} {Width of the AXI address bus.}
core_parameter AXI_DATA_WIDTH {AXI DATA WIDTH} {Width of the AXI data bus.}
core_parameter AXIS_TDATA_WIDTH {AXIS TDATA WIDTH} {Width of the S_AXIS data bus.}
core_parameter FIFO_WRITE_DEPTH {FIFO WRITE DEPTH} {Depth of the write side of the FIFO buffer.}

set address [ipx::get_address_spaces m_axi -of_objects $core]
set_property NAME M_AXI $address

set bus [ipx::get_bus_interfaces -of_objects $core m_axi]
set_property NAME M_AXI $bus
set_property INTERFACE_MODE master $bus
set_property MASTER_ADDRESS_SPACE_REF M_AXI $bus

set bus [ipx::get_bus_interfaces -of_objects $core s_axis]
set_property NAME S_AXIS $bus
set_property INTERFACE_MODE slave $bus

set bus [ipx::get_bus_interfaces aclk]
set parameter [ipx::get_bus_parameters -of_objects $bus ASSOCIATED_BUSIF]
set_property VALUE M_AXI:S_AXIS $parameter
