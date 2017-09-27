set_property PULLTYPE PULLUP [get_ports {exp_n_tri_io[0]}]
set_property PULLTYPE PULLUP [get_ports {exp_n_tri_io[1]}]
set_property PULLTYPE PULLUP [get_ports {exp_n_tri_io[2]}]
set_property PULLTYPE PULLUP [get_ports {exp_n_tri_io[3]}]

set_property IOSTANDARD LVCMOS33 [get_ports {exp_n_alex[*]}]
set_property SLEW FAST [get_ports {exp_n_alex[*]}]
set_property DRIVE 8 [get_ports {exp_n_alex[*]}]

set_property PACKAGE_PIN L15 [get_ports {exp_n_alex[0]}]
set_property PACKAGE_PIN L17 [get_ports {exp_n_alex[1]}]
set_property PACKAGE_PIN J16 [get_ports {exp_n_alex[2]}]
set_property PACKAGE_PIN M15 [get_ports {exp_n_alex[3]}]
