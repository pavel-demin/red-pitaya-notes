
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
  CFG_DATA_WIDTH 192
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
  DIN_WIDTH 192 DIN_FROM 0 DIN_TO 0 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_2 {
  DIN_WIDTH 192 DIN_FROM 1 DIN_TO 1 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_3 {
  DIN_WIDTH 192 DIN_FROM 16 DIN_TO 16 DOUT_WIDTH 1
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_4 {
  DIN_WIDTH 192 DIN_FROM 63 DIN_TO 32 DOUT_WIDTH 32
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_5 {
  DIN_WIDTH 192 DIN_FROM 79 DIN_TO 64 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_6 {
  DIN_WIDTH 192 DIN_FROM 127 DIN_TO 96 DOUT_WIDTH 32
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_7 {
  DIN_WIDTH 192 DIN_FROM 159 DIN_TO 128 DOUT_WIDTH 32
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_8 {
  DIN_WIDTH 192 DIN_FROM 191 DIN_TO 160 DOUT_WIDTH 32
} {
  Din cfg_0/cfg_data
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0

# Create axis_clock_converter
cell xilinx.com:ip:axis_clock_converter:1.1 fifo_0 {} {
  S_AXIS adc_0/M_AXIS
  s_axis_aclk adc_0/adc_clk
  s_axis_aresetn const_0/dout
  m_axis_aclk ps_0/FCLK_CLK0
  m_axis_aresetn rst_0/peripheral_aresetn
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter:1.1 subset_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 2
  TDATA_REMAP {tdata[31:16]}
} {
  S_AXIS fifo_0/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_constant
cell pavel-demin:user:axis_constant:1.0 phase_0 {
  AXIS_TDATA_WIDTH 32
} {
  cfg_data slice_4/Dout
  aclk ps_0/FCLK_CLK0
}

# Create dds_compiler
cell xilinx.com:ip:dds_compiler:6.0 dds_0 {
  DDS_CLOCK_RATE 125
  SPURIOUS_FREE_DYNAMIC_RANGE 138
  FREQUENCY_RESOLUTION 0.2
  PHASE_INCREMENT Streaming
  DSP48_USE Maximal
  HAS_TREADY true
  HAS_PHASE_OUT false
  PHASE_WIDTH 30
  OUTPUT_WIDTH 24
  NEGATIVE_SINE true
} {
  S_AXIS_PHASE phase_0/M_AXIS
  aclk ps_0/FCLK_CLK0
}

# Create axis_lfsr
cell pavel-demin:user:axis_lfsr:1.0 lfsr_0 {} {
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create cmpy
cell xilinx.com:ip:cmpy:6.0 mult_0 {
  FLOWCONTROL Blocking
  APORTWIDTH.VALUE_SRC USER
  BPORTWIDTH.VALUE_SRC USER
  APORTWIDTH 14
  BPORTWIDTH 24
  ROUNDMODE Random_Rounding
  OUTPUTWIDTH 25
} {
  S_AXIS_A subset_0/M_AXIS
  S_AXIS_B dds_0/M_AXIS_DATA
  S_AXIS_CTRL lfsr_0/M_AXIS
  aclk ps_0/FCLK_CLK0
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster:1.1 bcast_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 8
  M_TDATA_NUM_BYTES 3
  M00_TDATA_REMAP {tdata[23:0]}
  M01_TDATA_REMAP {tdata[55:32]}
} {
  S_AXIS mult_0/M_AXIS_DOUT
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_variable
cell pavel-demin:user:axis_variable:1.0 rate_0 {
  AXIS_TDATA_WIDTH 16
} {
  cfg_data slice_5/Dout
  aclk /ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_variable
cell pavel-demin:user:axis_variable:1.0 rate_1 {
  AXIS_TDATA_WIDTH 16
} {
  cfg_data slice_5/Dout
  aclk /ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create cic_compiler
cell xilinx.com:ip:cic_compiler:4.0 cic_0 {
  INPUT_DATA_WIDTH.VALUE_SRC USER
  FILTER_TYPE Decimation
  NUMBER_OF_STAGES 6
  SAMPLE_RATE_CHANGES Programmable
  MINIMUM_RATE 10
  MAXIMUM_RATE 25
  FIXED_OR_INITIAL_RATE 25
  INPUT_SAMPLE_FREQUENCY 125
  CLOCK_FREQUENCY 125
  INPUT_DATA_WIDTH 24
  QUANTIZATION Truncation
  OUTPUT_DATA_WIDTH 24
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
  MINIMUM_RATE 10
  MAXIMUM_RATE 25
  FIXED_OR_INITIAL_RATE 25
  INPUT_SAMPLE_FREQUENCY 125
  CLOCK_FREQUENCY 125
  INPUT_DATA_WIDTH 24
  QUANTIZATION Truncation
  OUTPUT_DATA_WIDTH 24
  USE_XTREME_DSP_SLICE false
} {
  S_AXIS_DATA bcast_0/M01_AXIS
  S_AXIS_CONFIG rate_1/M_AXIS
  aclk ps_0/FCLK_CLK0
}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner:1.1 comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 3
} {
  S00_AXIS cic_0/M_AXIS_DATA
  S01_AXIS cic_1/M_AXIS_DATA
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create fir_compiler
cell xilinx.com:ip:fir_compiler:7.2 fir_0 {
  DATA_WIDTH.VALUE_SRC USER
  DATA_WIDTH 24
  COEFFICIENTVECTOR {-6.31205679801719e-08, 7.30926012591262e-09, 5.58953452660316e-08, 4.87316950414411e-08, 1.07546133379516e-07, 3.01597505273952e-08, -4.29713011972205e-07, -5.15088683751126e-07, 6.65424773061835e-07, 1.61445611220616e-06, -2.65343636858081e-07, -3.19225955598462e-06, -1.4656956635344e-06, 4.51693256356331e-06, 4.95158733722241e-06, -4.26319378220919e-06, -9.7977498553796e-06, 9.44159216789214e-07, 1.441864781152e-05, 6.24458921740868e-06, -1.62108808252072e-05, -1.64853641255889e-05, 1.24879640578905e-05, 2.68673275185905e-05, -2.00749793274122e-06, -3.29542220537769e-05, -1.35587081091376e-05, 3.05974201003771e-05, 2.91704971507981e-05, -1.8518941384538e-05, -3.7799439825986e-05, 4.62942167460686e-07, 3.37382068187074e-05, 1.46141908228077e-05, -1.68363906144596e-05, -1.54744850104491e-05, -4.47829930408245e-06, -5.60815641581475e-06, 1.36919077773766e-05, 4.60682513345666e-05, 8.27297098324278e-06, -8.87336580662072e-05, -7.32338727071921e-05, 0.000103549328633743, 0.000175033457667912, -5.71933641796373e-05, -0.000283319306782567, -7.10198225130815e-05, 0.000347086648094344, 0.0002725529592572, -0.000310298185337442, -0.000500615805888086, 0.000136574477778254, 0.000676310580518881, 0.000165855478664172, -0.000712826035124893, -0.000531288259905026, 0.000552473370010762, 0.000846425444211122, -0.000203614400308839, -0.000987306081720199, -0.00024148557557037, 0.000874365380478607, 0.000622784687698624, -0.00052633212807923, -0.000767978042535689, 8.61930091998568e-05, 0.000575466705863226, 0.000203788903800764, -9.58942652276285e-05, -8.58738542248142e-05, -0.000426377291248088, -0.000586353574146813, 0.000586394287322406, 0.00171842961976755, 5.92652821292847e-05, -0.00289800600426928, -0.00179922254626698, 0.00342264027436982, 0.00455508633511873, -0.00246031750071, -0.00772680712889434, -0.000673365374407288, 0.0101676277313623, 0.00617506528822861, -0.0103448387509704, -0.0134514322246294, 0.00667744450716941, 0.0209459642925805, 0.0020053761720137, -0.026158551925879, -0.0159903667756223, 0.0258381410378789, 0.0342981867210979, -0.016190566750313, -0.0544180098277801, -0.00733626972481323, 0.0718523260750129, 0.0520124078773443, -0.0774924202696119, -0.137214106628167, 0.0359573137235839, 0.346857509696945, 0.503987573448888, 0.346857509696945, 0.0359573137235838, -0.137214106628167, -0.0774924202696119, 0.0520124078773442, 0.0718523260750129, -0.00733626972481316, -0.0544180098277801, -0.016190566750313, 0.0342981867210979, 0.0258381410378788, -0.0159903667756223, -0.026158551925879, 0.00200537617201369, 0.0209459642925805, 0.00667744450716941, -0.0134514322246294, -0.0103448387509704, 0.00617506528822858, 0.0101676277313623, -0.000673365374407245, -0.00772680712889434, -0.00246031750071, 0.00455508633511873, 0.0034226402743698, -0.00179922254626699, -0.00289800600426929, 5.92652821292939e-05, 0.00171842961976755, 0.000586394287322401, -0.000586353574146832, -0.000426377291248088, -8.58738542248089e-05, -9.58942652276304e-05, 0.000203788903800761, 0.000575466705863229, 8.61930091998532e-05, -0.000767978042535691, -0.000526332128079237, 0.000622784687698626, 0.000874365380478613, -0.000241485575570369, -0.000987306081720197, -0.00020361440030884, 0.000846425444211121, 0.000552473370010762, -0.000531288259905019, -0.000712826035124894, 0.000165855478664166, 0.000676310580518881, 0.000136574477778256, -0.000500615805888085, -0.000310298185337445, 0.000272552959257198, 0.000347086648094346, -7.10198225130807e-05, -0.000283319306782565, -5.71933641796392e-05, 0.00017503345766791, 0.000103549328633745, -7.32338727071911e-05, -8.87336580662082e-05, 8.27297098323674e-06, 4.60682513345666e-05, 1.36919077773794e-05, -5.60815641581445e-06, -4.47829930408515e-06, -1.54744850104495e-05, -1.68363906144591e-05, 1.46141908228082e-05, 3.37382068187067e-05, 4.62942167460444e-07, -3.77994398259859e-05, -1.85189413845379e-05, 2.9170497150799e-05, 3.05974201003769e-05, -1.35587081091375e-05, -3.2954222053777e-05, -2.00749793274087e-06, 2.68673275185905e-05, 1.24879640578904e-05, -1.64853641255888e-05, -1.62108808252074e-05, 6.24458921740866e-06, 1.44186478115199e-05, 9.44159216789226e-07, -9.79774985537956e-06, -4.26319378220916e-06, 4.95158733722243e-06, 4.51693256356326e-06, -1.46569566353445e-06, -3.19225955598459e-06, -2.65343636858069e-07, 1.61445611220614e-06, 6.65424773061808e-07, -5.15088683751126e-07, -4.29713011972229e-07, 3.01597505273901e-08, 1.07546133379525e-07, 4.87316950414471e-08, 5.5895345266022e-08, 7.30926012591106e-09, -6.31205679801768e-08}
  COEFFICIENT_WIDTH 24
  QUANTIZATION Quantize_Only
  BESTPRECISION true
  FILTER_TYPE Decimation
  DECIMATION_RATE 2
  NUMBER_PATHS 2
  SAMPLE_FREQUENCY 12.5
  CLOCK_FREQUENCY 125
  OUTPUT_ROUNDING_MODE Truncate_LSBs
  OUTPUT_WIDTH 25
} {
  S_AXIS_DATA comb_0/M_AXIS
  aclk ps_0/FCLK_CLK0
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster:1.1 bcast_1 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 8
  M_TDATA_NUM_BYTES 6
  M00_TDATA_REMAP {tdata[55:32],tdata[23:0]}
  M01_TDATA_REMAP {tdata[23:0],tdata[55:32]}
} {
  S_AXIS fir_0/M_AXIS_DATA
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_lfsr
cell pavel-demin:user:axis_lfsr:1.0 lfsr_1 {} {
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create cmpy
cell xilinx.com:ip:cmpy:6.0 mult_1 {
  FLOWCONTROL Blocking
  APORTWIDTH.VALUE_SRC USER
  BPORTWIDTH.VALUE_SRC USER
  APORTWIDTH 24
  BPORTWIDTH 24
  ROUNDMODE Random_Rounding
  OUTPUTWIDTH 33
} {
  S_AXIS_A bcast_1/M00_AXIS
  S_AXIS_B bcast_1/M01_AXIS
  S_AXIS_CTRL lfsr_1/M_AXIS
  aclk ps_0/FCLK_CLK0
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster:1.1 bcast_2 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 10
  M_TDATA_NUM_BYTES 4
  M00_TDATA_REMAP {tdata[71:40]}
  M01_TDATA_REMAP {tdata[71:40]}
} {
  S_AXIS mult_1/M_AXIS_DOUT
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_1 {
  CONST_WIDTH 32
  CONST_VAL 4294967295
}

# Create axis_trigger
cell pavel-demin:user:axis_trigger:1.0 trig_0 {
  AXIS_TDATA_WIDTH 32
  AXIS_TDATA_SIGNED TRUE
} {
  S_AXIS bcast_2/M00_AXIS
  pol_data slice_3/Dout
  msk_data const_1/Dout
  lvl_data slice_6/Dout
  aclk ps_0/FCLK_CLK0
}

# Create axis_packetizer
cell pavel-demin:user:axis_oscilloscope:1.0 scope_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 23
} {
  S_AXIS bcast_2/M01_AXIS
  run_flag slice_2/Dout
  trg_flag trig_0/trg_flag
  pre_data slice_7/Dout
  tot_data slice_8/Dout
  aclk ps_0/FCLK_CLK0
  aresetn slice_1/Dout
}

# Create axis_dwidth_converter
cell xilinx.com:ip:axis_dwidth_converter:1.1 conv_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 8
} {
  S_AXIS scope_0/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn slice_1/Dout
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_2 {
  CONST_WIDTH 32
  CONST_VAL 503316480
}

# Create axis_ram_writer
cell pavel-demin:user:axis_ram_writer:1.0 writer_0 {
  ADDR_WIDTH 22
} {
  S_AXIS conv_0/M_AXIS
  M_AXI ps_0/S_AXI_HP0
  cfg_data const_2/dout
  aclk ps_0/FCLK_CLK0
  aresetn slice_1/Dout
}

assign_bd_address [get_bd_addr_segs ps_0/S_AXI_HP0/HP0_DDR_LOWOCM]

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_0 {
  NUM_PORTS 2
  IN0_WIDTH 32
  IN1_WIDTH 32
} {
  In0 scope_0/sts_data
  In1 writer_0/sts_data
}

# Create axi_sts_register
cell pavel-demin:user:axi_sts_register:1.0 sts_0 {
  STS_DATA_WIDTH 64
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
