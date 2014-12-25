
### ADC

# data

set_property IOSTANDARD LVCMOS18 [get_ports {adc_data_a[*]}]
set_property IOB TRUE [get_ports {adc_data_a[*]}]

set_property PACKAGE_PIN Y17 [get_ports {adc_data_a[0]}]
set_property PACKAGE_PIN W16 [get_ports {adc_data_a[1]}]
set_property PACKAGE_PIN Y16 [get_ports {adc_data_a[2]}]
set_property PACKAGE_PIN W15 [get_ports {adc_data_a[3]}]
set_property PACKAGE_PIN W14 [get_ports {adc_data_a[4]}]
set_property PACKAGE_PIN Y14 [get_ports {adc_data_a[5]}]
set_property PACKAGE_PIN W13 [get_ports {adc_data_a[6]}]
set_property PACKAGE_PIN V12 [get_ports {adc_data_a[7]}]
set_property PACKAGE_PIN V13 [get_ports {adc_data_a[8]}]
set_property PACKAGE_PIN T14 [get_ports {adc_data_a[9]}]
set_property PACKAGE_PIN T15 [get_ports {adc_data_a[10]}]
set_property PACKAGE_PIN V15 [get_ports {adc_data_a[11]}]
set_property PACKAGE_PIN T16 [get_ports {adc_data_a[12]}]
set_property PACKAGE_PIN V16 [get_ports {adc_data_a[13]}]

set_property IOSTANDARD LVCMOS18 [get_ports {adc_data_b[*]}]
set_property IOB TRUE [get_ports {adc_data_b[*]}]

set_property PACKAGE_PIN R18 [get_ports {adc_data_b[0]}]
set_property PACKAGE_PIN P16 [get_ports {adc_data_b[1]}]
set_property PACKAGE_PIN P18 [get_ports {adc_data_b[2]}]
set_property PACKAGE_PIN N17 [get_ports {adc_data_b[3]}]
set_property PACKAGE_PIN R19 [get_ports {adc_data_b[4]}]
set_property PACKAGE_PIN T20 [get_ports {adc_data_b[5]}]
set_property PACKAGE_PIN T19 [get_ports {adc_data_b[6]}]
set_property PACKAGE_PIN U20 [get_ports {adc_data_b[7]}]
set_property PACKAGE_PIN V20 [get_ports {adc_data_b[8]}]
set_property PACKAGE_PIN W20 [get_ports {adc_data_b[9]}]
set_property PACKAGE_PIN W19 [get_ports {adc_data_b[10]}]
set_property PACKAGE_PIN Y19 [get_ports {adc_data_b[11]}]
set_property PACKAGE_PIN W18 [get_ports {adc_data_b[12]}]
set_property PACKAGE_PIN Y18 [get_ports {adc_data_b[13]}]

# clock input

set_property IOSTANDARD DIFF_HSTL_I_18 [get_ports adc_clk_p]
set_property IOSTANDARD DIFF_HSTL_I_18 [get_ports adc_clk_n]
set_property PACKAGE_PIN U18 [get_ports adc_clk_p]
set_property PACKAGE_PIN U19 [get_ports adc_clk_n]

# clock output

set_property IOSTANDARD LVCMOS18 [get_ports adc_enc_p]
set_property IOSTANDARD LVCMOS18 [get_ports adc_enc_n]

set_property SLEW FAST [get_ports adc_enc_p]
set_property SLEW FAST [get_ports adc_enc_n]

set_property DRIVE 8 [get_ports adc_enc_p]
set_property DRIVE 8 [get_ports adc_enc_n]

set_property PACKAGE_PIN N20 [get_ports adc_enc_p]
set_property PACKAGE_PIN P20 [get_ports adc_enc_n]

# clock duty cycle stabilizer (CSn)

set_property IOSTANDARD LVCMOS18 [get_ports adc_csn]
set_property PACKAGE_PIN V18 [get_ports adc_csn]
set_property SLEW FAST [get_ports adc_csn]
set_property DRIVE 8 [get_ports adc_csn]

### DAC

# data

set_property IOSTANDARD LVCMOS33 [get_ports {dac_data[*]}]
set_property SLEW SLOW [get_ports {dac_data[*]}]
set_property DRIVE 4 [get_ports {dac_data[*]}]

set_property PACKAGE_PIN M19 [get_ports {dac_data[0]}]
set_property PACKAGE_PIN M20 [get_ports {dac_data[1]}]
set_property PACKAGE_PIN L19 [get_ports {dac_data[2]}]
set_property PACKAGE_PIN L20 [get_ports {dac_data[3]}]
set_property PACKAGE_PIN K19 [get_ports {dac_data[4]}]
set_property PACKAGE_PIN J19 [get_ports {dac_data[5]}]
set_property PACKAGE_PIN J20 [get_ports {dac_data[6]}]
set_property PACKAGE_PIN H20 [get_ports {dac_data[7]}]
set_property PACKAGE_PIN G19 [get_ports {dac_data[8]}]
set_property PACKAGE_PIN G20 [get_ports {dac_data[9]}]
set_property PACKAGE_PIN F19 [get_ports {dac_data[10]}]
set_property PACKAGE_PIN F20 [get_ports {dac_data[11]}]
set_property PACKAGE_PIN D20 [get_ports {dac_data[12]}]
set_property PACKAGE_PIN D19 [get_ports {dac_data[13]}]

# control

set_property IOSTANDARD LVCMOS33 [get_ports dac_*_o]
set_property SLEW FAST [get_ports dac_*_o]
set_property DRIVE 8 [get_ports dac_*_o]

set_property PACKAGE_PIN M17 [get_ports dac_wrt_o]
set_property PACKAGE_PIN N16 [get_ports dac_sel_o]
set_property PACKAGE_PIN M18 [get_ports dac_clk_o]
set_property PACKAGE_PIN N15 [get_ports dac_rst_o]

### PWM

set_property IOSTANDARD LVCMOS18 [get_ports {pwm[*]}]
set_property SLEW FAST [get_ports {pwm[*]}]
set_property DRIVE 12 [get_ports {pwm[*]}]
set_property IOB TRUE [get_ports {pwm[*]}]

set_property PACKAGE_PIN T10 [get_ports {pwm[0]}]
set_property PACKAGE_PIN T11 [get_ports {pwm[1]}]
set_property PACKAGE_PIN P15 [get_ports {pwm[2]}]
set_property PACKAGE_PIN U13 [get_ports {pwm[3]}]

### XADC

set_property IOSTANDARD LVCMOS33 [get_ports {vin_p[*]}]
set_property IOSTANDARD LVCMOS33 [get_ports {vin_n[*]}]
set_property LOC XADC_X0Y0 [get_cells i_ams/XADC_inst]

# AD0

set_property PACKAGE_PIN C20 [get_ports {vin_p[0]}]
set_property PACKAGE_PIN B20 [get_ports {vin_n[0]}]

# AD1

set_property PACKAGE_PIN E17 [get_ports {vin_p[1]}]
set_property PACKAGE_PIN D18 [get_ports {vin_n[1]}]

# AD8

set_property PACKAGE_PIN B19 [get_ports {vin_p[2]}]
set_property PACKAGE_PIN A20 [get_ports {vin_n[2]}]

# AD9

set_property PACKAGE_PIN E18 [get_ports {vin_p[3]}]
set_property PACKAGE_PIN E19 [get_ports {vin_n[3]}]

# V_0

set_property PACKAGE_PIN K9  [get_ports {vin_p[4]}]
set_property PACKAGE_PIN L10 [get_ports {vin_n[4]}]

### Expansion connector

set_property IOSTANDARD LVCMOS33 [get_ports {exp_p[*]}]
set_property IOSTANDARD LVCMOS33 [get_ports {exp_n[*]}]
set_property SLEW FAST [get_ports {exp_p[*]}]
set_property SLEW FAST [get_ports {exp_n[*]}]
set_property DRIVE 8 [get_ports {exp_p[*]}]
set_property DRIVE 8 [get_ports {exp_n[*]}]

set_property PACKAGE_PIN G17 [get_ports {exp_p[0]}]
set_property PACKAGE_PIN G18 [get_ports {exp_n[0]}]
set_property PACKAGE_PIN H16 [get_ports {exp_p[1]}]
set_property PACKAGE_PIN H17 [get_ports {exp_n[1]}]
set_property PACKAGE_PIN J18 [get_ports {exp_p[2]}]
set_property PACKAGE_PIN H18 [get_ports {exp_n[2]}]
set_property PACKAGE_PIN K17 [get_ports {exp_p[3]}]
set_property PACKAGE_PIN K18 [get_ports {exp_n[3]}]
set_property PACKAGE_PIN L14 [get_ports {exp_p[4]}]
set_property PACKAGE_PIN L15 [get_ports {exp_n[4]}]
set_property PACKAGE_PIN L16 [get_ports {exp_p[5]}]
set_property PACKAGE_PIN L17 [get_ports {exp_n[5]}]
set_property PACKAGE_PIN K16 [get_ports {exp_p[6]}]
set_property PACKAGE_PIN J16 [get_ports {exp_n[6]}]
set_property PACKAGE_PIN M14 [get_ports {exp_p[7]}]
set_property PACKAGE_PIN M15 [get_ports {exp_n[7]}]

### SATA connector

set_property PACKAGE_PIN T12 [get_ports {sata_p[0]}]
set_property PACKAGE_PIN U12 [get_ports {sata_n[0]}]

set_property PACKAGE_PIN U14 [get_ports {sata_p[1]}]
set_property PACKAGE_PIN U15 [get_ports {sata_n[1]}]

set_property PACKAGE_PIN P14 [get_ports {sata_p[2]}]
set_property PACKAGE_PIN R14 [get_ports {sata_n[2]}]

set_property PACKAGE_PIN N18 [get_ports {sata_p[3]}]
set_property PACKAGE_PIN P19 [get_ports {sata_n[3]}]

### LED

set_property IOSTANDARD LVCMOS33 [get_ports {led[*]}]
set_property SLEW SLOW [get_ports {led[*]}]
set_property DRIVE 8 [get_ports {led[*]}]

set_property PACKAGE_PIN F16 [get_ports {led[0]}]
set_property PACKAGE_PIN F17 [get_ports {led[1]}]
set_property PACKAGE_PIN G15 [get_ports {led[2]}]
set_property PACKAGE_PIN H15 [get_ports {led[3]}]
set_property PACKAGE_PIN K14 [get_ports {led[4]}]
set_property PACKAGE_PIN G14 [get_ports {led[5]}]
set_property PACKAGE_PIN J15 [get_ports {led[6]}]
set_property PACKAGE_PIN J14 [get_ports {led[7]}]
