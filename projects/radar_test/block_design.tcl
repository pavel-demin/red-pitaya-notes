# Create processing_system7
cell xilinx.com:ip:processing_system7:5.5 ps_0 {
  PCW_IMPORT_BOARD_PRESET cfg/red_pitaya.xml
  PCW_USE_S_AXI_HP0 1
} {
  M_AXI_GP0_ACLK ps_0/FCLK_CLK0
  S_AXI_HP0_ACLK ps_0/FCLK_CLK0
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {
  make_external {FIXED_IO, DDR}
  Master Disable
  Slave Disable
} [get_bd_cells ps_0]

# Create proc_sys_reset
cell xilinx.com:ip:proc_sys_reset:5.0 rst_0

# Create util_ds_buf
cell xilinx.com:ip:util_ds_buf:2.1 buf_0 {
  C_SIZE 2
  C_BUF_TYPE IBUFDS
} {
  IBUF_DS_P daisy_p_i
  IBUF_DS_N daisy_n_i
}

# Create util_ds_buf
cell xilinx.com:ip:util_ds_buf:2.1 buf_1 {
  C_SIZE 2
  C_BUF_TYPE OBUFDS
} {
  OBUF_DS_P daisy_p_o
  OBUF_DS_N daisy_n_o
}

# Create axis_red_pitaya_adc
cell pavel-demin:user:axis_red_pitaya_adc:1.0 adc_0 {} {
  adc_clk_p adc_clk_p_i
  adc_clk_n adc_clk_n_i
  adc_dat_a adc_dat_a_i
  adc_dat_b adc_dat_b_i
  adc_csn adc_csn_o
}

# Create axis_gpio_reader
cell pavel-demin:user:axis_gpio_reader:1.0 gpio_0 {
  AXIS_TDATA_WIDTH 8
} {
  gpio_data exp_p_io
  aclk adc_0/adc_clk
}

# Create c_counter_binary
cell xilinx.com:ip:c_counter_binary:12.0 cntr_0 {
  Output_Width 32
} {
  CLK adc_0/adc_clk
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_0 {
  DIN_WIDTH 32 DIN_FROM 26 DIN_TO 26 DOUT_WIDTH 1
} {
  Din cntr_0/Q
  Dout led_o
}

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register:1.0 cfg_0 {
  CFG_DATA_WIDTH 96
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins cfg_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_cfg_0_reg0]
set_property OFFSET 0x40000000 [get_bd_addr_segs ps_0/Data/SEG_cfg_0_reg0]

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_1 {
  DIN_WIDTH 96 DIN_FROM 0 DIN_TO 0 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_2 {
  DIN_WIDTH 96 DIN_FROM 16 DIN_TO 16 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_3 {
  DIN_WIDTH 96 DIN_FROM 47 DIN_TO 32 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_4 {
  DIN_WIDTH 96 DIN_FROM 63 DIN_TO 48 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_5 {
  DIN_WIDTH 96 DIN_FROM 79 DIN_TO 64 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_6 {
  DIN_WIDTH 96 DIN_FROM 95 DIN_TO 80 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner:1.1 comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
} {
  S00_AXIS adc_0/M_AXIS
  S01_AXIS gpio_0/M_AXIS
  aclk adc_0/adc_clk
  aresetn const_0/dout
}

# Create axis_clock_converter
cell xilinx.com:ip:axis_clock_converter:1.1 fifo_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 5
} {
  S_AXIS comb_0/M_AXIS
  s_axis_aclk adc_0/adc_clk
  s_axis_aresetn const_0/dout
  m_axis_aclk ps_0/FCLK_CLK0
  m_axis_aresetn rst_0/peripheral_aresetn
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster:1.1 bcast_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 6
  M_TDATA_NUM_BYTES 2
  NUM_MI 3
  M00_TDATA_REMAP {tdata[15:0]}
  M01_TDATA_REMAP {tdata[31:16]}
  M02_TDATA_REMAP {tdata[47:32]}
} {
  S_AXIS fifo_0/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_variable
cell pavel-demin:user:axis_variable:1.0 rate_0 {
  AXIS_TDATA_WIDTH 16
} {
  cfg_data slice_6/Dout
  aclk /ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_variable
cell pavel-demin:user:axis_variable:1.0 rate_1 {
  AXIS_TDATA_WIDTH 16
} {
  cfg_data slice_6/Dout
  aclk /ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create cic_compiler
cell xilinx.com:ip:cic_compiler:4.0 cic_0 {
  INPUT_DATA_WIDTH.VALUE_SRC USER
  FILTER_TYPE Decimation
  NUMBER_OF_STAGES 6
  SAMPLE_RATE_CHANGES Programmable
  MINIMUM_RATE 5
  MAXIMUM_RATE 125
  FIXED_OR_INITIAL_RATE 5
  INPUT_SAMPLE_FREQUENCY 125
  CLOCK_FREQUENCY 125
  INPUT_DATA_WIDTH 14
  QUANTIZATION Truncation
  OUTPUT_DATA_WIDTH 16
  USE_XTREME_DSP_SLICE false
} {
  S_AXIS_DATA bcast_0/M00_AXIS
  S_AXIS_CONFIG rate_0/M_AXIS
  aclk ps_0/FCLK_CLK0
}

# Create cic_compiler
cell xilinx.com:ip:cic_compiler:4.0 cic_1 {
  INPUT_DATA_WIDTH.VALUE_SRC USER
  FILTER_TYPE Decimation
  NUMBER_OF_STAGES 6
  SAMPLE_RATE_CHANGES Programmable
  MINIMUM_RATE 5
  MAXIMUM_RATE 125
  FIXED_OR_INITIAL_RATE 5
  INPUT_SAMPLE_FREQUENCY 125
  CLOCK_FREQUENCY 125
  INPUT_DATA_WIDTH 14
  QUANTIZATION Truncation
  OUTPUT_DATA_WIDTH 16
  USE_XTREME_DSP_SLICE false
} {
  S_AXIS_DATA bcast_0/M01_AXIS
  S_AXIS_CONFIG rate_1/M_AXIS
  aclk ps_0/FCLK_CLK0
}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner:1.1 comb_1 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 2
} {
  S00_AXIS cic_0/M_AXIS_DATA
  S01_AXIS cic_1/M_AXIS_DATA
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create fir_compiler
cell xilinx.com:ip:fir_compiler:7.2 fir_0 {
  DATA_WIDTH.VALUE_SRC USER
  DATA_WIDTH 16
  COEFFICIENTVECTOR {-6.20513760827418e-08, 7.73963117002126e-09, 5.4978346298851e-08, 4.69162790035053e-08, 1.05729689795557e-07, 3.15193742788112e-08, -4.22379182007162e-07, -5.10012199072496e-07, 6.53269697377322e-07, 1.59367278327052e-06, -2.5708338020049e-07, -3.14822505487741e-06, -1.45109806435614e-06, 4.4525802566908e-06, 4.88886387066429e-06, -4.20064529396924e-06, -9.66734618192478e-06, 9.27004607474561e-07, 1.42241810987294e-05, 6.1615271189161e-06, -1.59938891711412e-05, -1.62601189719496e-05, 1.23286237751205e-05, 2.65016493112049e-05, -2.00257386986528e-06, -3.25156515042921e-05, -1.33396649753619e-05, 3.02140233243377e-05, 2.87353687739199e-05, -1.83353524630811e-05, -3.72626990698873e-05, 5.6434609191222e-07, 3.3300220450176e-05, 1.4280751295588e-05, -1.66988238006688e-05, -1.51341197021717e-05, -4.24178958855455e-06, -5.61072959382936e-06, 1.32577420447096e-05, 4.54228242152688e-05, 8.43886738956731e-06, -8.73831123911003e-05, -7.24833302792887e-05, 0.000101882322461832, 0.000172802863263319, -5.60934850597305e-05, -0.000279488333526277, -7.03615955491941e-05, 0.000342282982616864, 0.000269071561690636, -0.000305968475411435, -0.000493937137582533, 0.000134687602259139, 0.000667221066809241, 0.000163474134077809, -0.000703378257081798, -0.000523808607508423, 0.000545539548375308, 0.000834712673998596, -0.000201887086520397, -0.000974056860930524, -0.000236747417159668, 0.000863420377954923, 0.000612758853260792, -0.000521184972455496, -0.000756442246729674, 8.81190392060157e-05, 0.000567714940512866, 0.00019721787931471, -9.64355480367413e-05, -8.1009490353649e-05, -0.00041672945537519, -0.000580917804838544, 0.000572919021465635, 0.00169546933662316, 6.46440696774492e-05, -0.00285640175564832, -0.00178028318261647, 0.00337164992078623, 0.00449681129220342, -0.00242138748583869, -0.00762313133128295, -0.000668969479458563, 0.0100294942908157, 0.00609381194583225, -0.0102056321797817, -0.0132695974279251, 0.00659309913358623, 0.0206642774357002, 0.00196360077588811, -0.0258164610821417, -0.0157511181462702, 0.0255249192763533, 0.0338130429337539, -0.0160523288123454, -0.0536940109781236, -0.00709065748680499, 0.071008809418442, 0.0511002138960635, -0.0769236758485952, -0.135317482075359, 0.0370795004025239, 0.345622440863094, 0.501457142963004, 0.345622440863094, 0.0370795004025239, -0.135317482075359, -0.0769236758485952, 0.0511002138960635, 0.071008809418442, -0.00709065748680493, -0.0536940109781236, -0.0160523288123454, 0.0338130429337539, 0.0255249192763532, -0.0157511181462702, -0.0258164610821417, 0.0019636007758881, 0.0206642774357002, 0.00659309913358623, -0.0132695974279251, -0.0102056321797817, 0.00609381194583222, 0.0100294942908157, -0.000668969479458522, -0.00762313133128295, -0.0024213874858387, 0.00449681129220343, 0.00337164992078622, -0.00178028318261647, -0.00285640175564833, 6.46440696774579e-05, 0.00169546933662317, 0.00057291902146563, -0.000580917804838557, -0.00041672945537519, -8.10094903536513e-05, -9.6435548036744e-05, 0.000197217879314706, 0.000567714940512869, 8.8119039206013e-05, -0.000756442246729676, -0.000521184972455502, 0.000612758853260793, 0.000863420377954929, -0.000236747417159668, -0.000974056860930522, -0.000201887086520398, 0.000834712673998598, 0.000545539548375309, -0.000523808607508415, -0.000703378257081798, 0.0001634741340778, 0.000667221066809241, 0.00013468760225914, -0.000493937137582534, -0.000305968475411438, 0.000269071561690634, 0.000342282982616867, -7.03615955491928e-05, -0.000279488333526276, -5.60934850597328e-05, 0.000172802863263316, 0.000101882322461835, -7.24833302792876e-05, -8.73831123911015e-05, 8.43886738955908e-06, 4.54228242152688e-05, 1.32577420447137e-05, -5.61072959382912e-06, -4.24178958855691e-06, -1.51341197021725e-05, -1.66988238006683e-05, 1.42807512955888e-05, 3.33002204501756e-05, 5.64346091911769e-07, -3.72626990698874e-05, -1.8335352463081e-05, 2.87353687739206e-05, 3.02140233243375e-05, -1.33396649753618e-05, -3.25156515042921e-05, -2.00257386986519e-06, 2.65016493112049e-05, 1.23286237751207e-05, -1.62601189719496e-05, -1.59938891711414e-05, 6.16152711891607e-06, 1.42241810987293e-05, 9.2700460747459e-07, -9.66734618192474e-06, -4.20064529396921e-06, 4.88886387066428e-06, 4.45258025669078e-06, -1.45109806435621e-06, -3.14822505487736e-06, -2.57083380200441e-07, 1.59367278327051e-06, 6.53269697377264e-07, -5.10012199072496e-07, -4.22379182007174e-07, 3.15193742788041e-08, 1.05729689795557e-07, 4.69162790035126e-08, 5.49783462988405e-08, 7.73963117002046e-09, -6.20513760827445e-08}
  COEFFICIENT_WIDTH 16
  QUANTIZATION Quantize_Only
  BESTPRECISION true
  FILTER_TYPE Decimation
  DECIMATION_RATE 2
  NUMBER_PATHS 2
  SAMPLE_FREQUENCY 25
  CLOCK_FREQUENCY 125
  OUTPUT_ROUNDING_MODE Truncate_LSBs
  OUTPUT_WIDTH 16
} {
  S_AXIS_DATA comb_1/M_AXIS
  aclk ps_0/FCLK_CLK0
}

# Create axis_trigger
cell pavel-demin:user:axis_trigger:1.0 trig_0 {
  AXIS_TDATA_WIDTH 8
  AXIS_TDATA_SIGNED FALSE
} {
  S_AXIS bcast_0/M02_AXIS
  pol_data slice_2/Dout
  lvl_data slice_3/Dout
  aclk ps_0/FCLK_CLK0
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_1

# Create axis_packetizer
cell pavel-demin:user:axis_oscilloscope:1.0 scope_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 14
} {
  S_AXIS fir_0/M_AXIS_DATA
  run_flag trig_0/trg_flag
  trg_flag const_1/dout
  pre_data slice_4/Dout
  tot_data slice_5/Dout
  aclk ps_0/FCLK_CLK0
  aresetn slice_1/Dout
}

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen:8.2 bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  USE_BYTE_WRITE_ENABLE true
  BYTE_SIZE 8
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 16384
  WRITE_WIDTH_B 32
  WRITE_DEPTH_B 16384
  ENABLE_A Always_Enabled
  ENABLE_B Always_Enabled
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
}

# Create axis_bram_writer
cell pavel-demin:user:axis_bram_writer:1.0 writer_0 {
  AXIS_TDATA_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 14
} {
  S_AXIS scope_0/M_AXIS
  BRAM_PORTA bram_0/BRAM_PORTA
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axi_bram_reader
cell pavel-demin:user:axi_bram_reader:1.0 reader_0 {
  AXI_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 14
} {
  BRAM_PORTA bram_0/BRAM_PORTB
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins reader_0/S_AXI]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]
set_property OFFSET 0x40010000 [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_0 {
  NUM_PORTS 2
  IN0_WIDTH 16
  IN1_WIDTH 16
} {
  In0 scope_0/sts_data
  In1 writer_0/sts_data
}

# Create axi_sts_register
cell pavel-demin:user:axi_sts_register:1.0 sts_0 {
  STS_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
} {
  sts_data concat_0/dout
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins sts_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_sts_0_reg0]
set_property OFFSET 0x40001000 [get_bd_addr_segs ps_0/Data/SEG_sts_0_reg0]