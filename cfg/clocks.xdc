create_clock -period 8.000 -name adc_clk [get_ports adc_clk_p]

set_input_delay -clock adc_clk 3.400 [get_ports adc_data_a[*]]
set_input_delay -clock adc_clk 3.400 [get_ports adc_data_b[*]]

