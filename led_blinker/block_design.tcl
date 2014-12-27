source base_system/block_design.tcl

# Create axis_red_pitaya_adc
create_bd_cell -vlnv pavel-demin:user:axis_red_pitaya_adc:1.0 adc_0

# Connect axis_red_pitaya_adc to interface ports
connect_bd_net [get_bd_pins adc_0/adc_clk_p] [get_bd_ports adc_clk_p_i]
connect_bd_net [get_bd_pins adc_0/adc_clk_n] [get_bd_ports adc_clk_n_i]
connect_bd_net [get_bd_pins adc_0/adc_dat_a] [get_bd_ports adc_dat_a_i]
connect_bd_net [get_bd_pins adc_0/adc_dat_b] [get_bd_ports adc_dat_b_i]
connect_bd_net [get_bd_pins adc_0/adc_csn] [get_bd_ports adc_csn_o]

# Create c_counter_binary
create_bd_cell -vlnv xilinx.com:ip:c_counter_binary:12.0 cntr_0
set_property CONFIG.Output_Width 32 [get_bd_cells cntr_0]

# Connect axis_red_pitaya_adc to c_counter_binary
connect_bd_net [get_bd_pins adc_0/adc_clk] [get_bd_pins cntr_0/CLK]

# Create xlslice
create_bd_cell -vlnv xilinx.com:ip:xlslice:1.0 slice_0
set_property CONFIG.DIN_FROM 26 [get_bd_cells slice_0]
set_property CONFIG.DIN_TO 26 [get_bd_cells slice_0]

# Connect c_counter_binary to xlslice
connect_bd_net [get_bd_pins cntr_0/Q] [get_bd_pins slice_0/Din]

# Connect xlslice to interface port
connect_bd_net [get_bd_ports led_o] [get_bd_pins slice_0/Dout]

# Create hierarchical module
create_bd_cell -type hier blinker_0

# Move cntr_0 and slice_0 to blinker_0
move_bd_cells blinker_0 [get_bd_cells cntr_0] [get_bd_cells slice_0]
