### SATA connector

set_property IOSTANDARD DIFF_HSTL_I_18 [get_ports daisy_trg*]
set_property IOSTANDARD DIFF_HSTL_I_18 [get_ports daisy_clk*]

set_property PACKAGE_PIN T12 [get_ports daisy_trg0_p_o]
set_property PACKAGE_PIN U12 [get_ports daisy_trg0_n_o]

set_property PACKAGE_PIN U14 [get_ports daisy_clk0_p_o]
set_property PACKAGE_PIN U15 [get_ports daisy_clk0_n_o]

set_property PACKAGE_PIN P14 [get_ports daisy_trg1_p_o]
set_property PACKAGE_PIN R14 [get_ports daisy_trg1_n_o]

set_property PACKAGE_PIN N18 [get_ports daisy_clk1_p_o]
set_property PACKAGE_PIN P19 [get_ports daisy_clk1_n_o]
