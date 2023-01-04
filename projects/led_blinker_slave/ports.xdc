### SATA connector

set_property IOSTANDARD DIFF_HSTL_I_18 [get_ports daisy_trg*]
set_property IOSTANDARD DIFF_HSTL_I_18 [get_ports daisy_clk*]

set_property PACKAGE_PIN T12 [get_ports daisy_trg_p_i]
set_property PACKAGE_PIN U12 [get_ports daisy_trg_n_i]

set_property PACKAGE_PIN U14 [get_ports daisy_clk_p_i]
set_property PACKAGE_PIN U15 [get_ports daisy_clk_n_i]

set_property PACKAGE_PIN P14 [get_ports daisy_trg_p_o]
set_property PACKAGE_PIN R14 [get_ports daisy_trg_n_o]

set_property PACKAGE_PIN N18 [get_ports daisy_clk_p_o]
set_property PACKAGE_PIN P19 [get_ports daisy_clk_n_o]

### clock output

set_property IOSTANDARD DIFF_HSTL_I_18 [get_ports adc_clk_p_o]
set_property IOSTANDARD DIFF_HSTL_I_18 [get_ports adc_clk_n_o]

set_property PACKAGE_PIN N20 [get_ports adc_clk_p_o]
set_property PACKAGE_PIN P20 [get_ports adc_clk_n_o]
