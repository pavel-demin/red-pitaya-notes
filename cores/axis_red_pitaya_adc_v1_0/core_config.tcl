set display_name {AXI4-Stream Red Pitaya ADC}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

set bus [ipx::get_bus_interfaces adc_signal_clock]
set parameter [ipx::add_bus_parameter ASSOCIATED_BUSIF $bus]
set_property VALUE M_AXIS $parameter

core_parameter AXIS_TDATA_WIDTH {AXIS TDATA WIDTH} {Width of the M_AXIS data bus.}

core_parameter ADC_DATA_WIDTH {ADC DATA WIDTH} {Width of the ADC data bus.}

