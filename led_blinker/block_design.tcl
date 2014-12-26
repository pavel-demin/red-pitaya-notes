source base_system/block_design.tcl

# Create axis_red_pitaya_adc
create_bd_cell -type ip -vlnv pavel-demin:user:axis_red_pitaya_adc:1.0 adc_0

# Create interface ports for axis_red_pitaya_adc
create_bd_port -dir I adc_clk_n
create_bd_port -dir I adc_clk_p
create_bd_port -dir I -from 13 -to 0 adc_data_a
create_bd_port -dir I -from 13 -to 0 adc_data_b
create_bd_port -dir O adc_csn

# Connect axis_red_pitaya_adc to interface ports
connect_bd_net [get_bd_ports adc_clk_n] [get_bd_pins adc_0/adc_clk_n]
connect_bd_net [get_bd_ports adc_clk_p] [get_bd_pins adc_0/adc_clk_p]
connect_bd_net [get_bd_ports adc_data_a] [get_bd_pins adc_0/adc_data_a]
connect_bd_net [get_bd_ports adc_data_b] [get_bd_pins adc_0/adc_data_b]
connect_bd_net [get_bd_ports adc_csn] [get_bd_pins adc_0/adc_csn]

# Create c_counter_binary
create_bd_cell -type ip -vlnv xilinx.com:ip:c_counter_binary:12.0 cntr_0
set_property -dict [list CONFIG.Output_Width {32}] [get_bd_cells cntr_0]

# Connect axis_red_pitaya_adc to c_counter_binary
connect_bd_net [get_bd_pins adc_0/adc_clk] [get_bd_pins cntr_0/CLK]

# Create xlslice
create_bd_cell -type ip -vlnv xilinx.com:ip:xlslice:1.0 slice_0
set_property -dict [list CONFIG.DIN_FROM {26} CONFIG.DIN_TO {26}] [get_bd_cells slice_0]

# Connect c_counter_binary to xlslice
connect_bd_net [get_bd_pins cntr_0/Q] [get_bd_pins slice_0/Din]

# Create interface port for xlslice
create_bd_port -dir O -from 0 -to 0 led

# Connect xlslice to interface port
connect_bd_net [get_bd_ports led] [get_bd_pins slice_0/Dout]
