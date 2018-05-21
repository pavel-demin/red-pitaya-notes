set display_name {Port Selector}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

core_parameter DOUT_WIDTH {DOUT WIDTH} {Width of the output port.}
