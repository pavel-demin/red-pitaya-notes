set display_name {Pulse Generator}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

core_parameter CONTINUOUS {CONTINUOUS} {If TRUE, pulse generator runs continuously.}
