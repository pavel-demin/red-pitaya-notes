set display_name {Shift Register}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

core_parameter DATA_WIDTH {DATA WIDTH} {Width of the input and output ports.}
