set display_name {Port Slicer}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

core_parameter DIN_WIDTH {DIN WIDTH} {Width of the input port.}
core_parameter DIN_FROM {DIN FROM} {Index of the highest selected bit.}
core_parameter DIN_TO {DIN TO} {Index of the lowest selected bit.}
