set display_name {DSP48}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

core_parameter A_WIDTH {A WIDTH} {Width of the input port A.}
core_parameter B_WIDTH {B WIDTH} {Width of the input port B.}
core_parameter P_WIDTH {P WIDTH} {Width of the output port P.}
