# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_0 {
  DIN_WIDTH 8 DIN_FROM 1 DIN_TO 1
}

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_1 {
  DIN_WIDTH 8 DIN_FROM 2 DIN_TO 2
}

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_2 {
  DIN_WIDTH 96 DIN_FROM 15 DIN_TO 0
}

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_3 {
  DIN_WIDTH 96 DIN_FROM 63 DIN_TO 32
}

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_4 {
  DIN_WIDTH 96 DIN_FROM 79 DIN_TO 64
}

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_5 {
  DIN_WIDTH 96 DIN_FROM 95 DIN_TO 80
}

# Create axis_constant
cell pavel-demin:user:axis_constant:1.0 phase_0 {
  AXIS_TDATA_WIDTH 32
} {
  cfg_data slice_3/dout
  aclk /pll_0/clk_out1
}

# Create dds_compiler
cell xilinx.com:ip:dds_compiler:6.0 dds_0 {
  DDS_CLOCK_RATE 125
  SPURIOUS_FREE_DYNAMIC_RANGE 138
  FREQUENCY_RESOLUTION 0.2
  PHASE_INCREMENT Streaming
  HAS_PHASE_OUT false
  PHASE_WIDTH 30
  OUTPUT_WIDTH 24
  DSP48_USE Minimal
  NEGATIVE_SINE true
} {
  S_AXIS_PHASE phase_0/M_AXIS
  aclk /pll_0/clk_out1
}

# Create axis_lfsr
cell pavel-demin:user:axis_lfsr:1.0 lfsr_0 {} {
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0

for {set i 0} {$i <= 1} {incr i} {

  # Create xlslice
  cell pavel-demin:user:port_slicer:1.0 adc_slice_$i {
    DIN_WIDTH 96 DIN_FROM 13 DIN_TO 0
  } {
    din /adc_0/m_axis_tdata
  }

  # Create xlslice
  cell pavel-demin:user:port_slicer:1.0 dds_slice_$i {
    DIN_WIDTH 48 DIN_FROM [expr 24 * $i + 20] DIN_TO [expr 24 * $i]
  } {
    din dds_0/m_axis_data_tdata
  }

  # Create xbip_dsp48_macro
  cell xilinx.com:ip:xbip_dsp48_macro:3.0 mult_$i {
    INSTRUCTION1 RNDSIMPLE(A*B+CARRYIN)
    A_WIDTH.VALUE_SRC USER
    B_WIDTH.VALUE_SRC USER
    OUTPUT_PROPERTIES User_Defined
    A_WIDTH 21
    B_WIDTH 14
    P_WIDTH 25
  } {
    A dds_slice_$i/dout
    B adc_slice_$i/dout
    CARRYIN lfsr_0/m_axis_tdata
    CLK /pll_0/clk_out1
  }

  # Create axis_variable
  cell pavel-demin:user:axis_variable:1.0 rate_$i {
    AXIS_TDATA_WIDTH 16
  } {
    cfg_data slice_2/dout
    aclk /pll_0/clk_out1
    aresetn /rst_0/peripheral_aresetn
  }

  # Create cic_compiler
  cell xilinx.com:ip:cic_compiler:4.0 cic_$i {
    INPUT_DATA_WIDTH.VALUE_SRC USER
    FILTER_TYPE Decimation
    NUMBER_OF_STAGES 6
    SAMPLE_RATE_CHANGES Programmable
    MINIMUM_RATE 125
    MAXIMUM_RATE 5000
    FIXED_OR_INITIAL_RATE 125
    INPUT_SAMPLE_FREQUENCY 125
    CLOCK_FREQUENCY 125
    INPUT_DATA_WIDTH 24
    QUANTIZATION Truncation
    OUTPUT_DATA_WIDTH 24
    USE_XTREME_DSP_SLICE false
    HAS_ARESETN true
  } {
    s_axis_data_tdata mult_$i/P
    s_axis_data_tvalid const_0/dout
    S_AXIS_CONFIG rate_$i/M_AXIS
    aclk /pll_0/clk_out1
    aresetn /rst_0/peripheral_aresetn
  }

}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner:1.1 comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 3
} {
  S00_AXIS cic_0/M_AXIS_DATA
  S01_AXIS cic_1/M_AXIS_DATA
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_dwidth_converter
cell xilinx.com:ip:axis_dwidth_converter:1.1 conv_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 18
  M_TDATA_NUM_BYTES 3
} {
  S_AXIS comb_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create fir_compiler
cell xilinx.com:ip:fir_compiler:7.2 fir_0 {
  DATA_WIDTH.VALUE_SRC USER
  DATA_WIDTH 24
  COEFFICIENTVECTOR {-1.6476043851e-08, -4.7317815530e-08, -7.9303729227e-10, 3.0931165073e-08, 1.8625056624e-08, 3.2745385737e-08, -6.2984765306e-09, -1.5226511473e-07, -8.3038042570e-08, 3.1450503208e-07, 3.0559714131e-07, -4.7412677515e-07, -7.1342595428e-07, 5.4727096517e-07, 1.3344925862e-06, -4.1410064947e-07, -2.1503018057e-06, -6.7734441506e-08, 3.0751670252e-06, 1.0369221285e-06, -3.9439819633e-06, -2.5916621140e-06, 4.5149379235e-06, 4.7473659764e-06, -4.4924073693e-06, -7.3975295122e-06, 3.5718171133e-06, 1.0288534956e-05, -1.5036743791e-06, -1.3019605998e-05, -1.8319006549e-06, 1.5076697696e-05, 6.3540333125e-06, -1.5904152941e-05, -1.1731346065e-05, 1.5009560931e-05, 1.7370299240e-05, -1.2093181242e-05, -2.2464707024e-05, 7.1690491210e-06, 2.6100740650e-05, -6.6351181774e-07, -2.7426623195e-05, -6.5499888643e-06, 2.5861850176e-05, 1.3203036531e-05, -2.1314973380e-05, -1.7788292260e-05, 1.4365010410e-05, 1.8817826302e-05, -6.3570800745e-06, -1.5160998822e-05, -6.3448606907e-07, 6.4151241761e-06, 4.0074018945e-06, 6.7569489549e-06, -1.0052830942e-06, -2.2400853889e-05, -1.0761588005e-05, 3.7229777761e-05, 3.2697649361e-05, -4.6855979020e-05, -6.4646397163e-05, 4.6255508155e-05, 1.0439175120e-04, -3.0537437275e-05, -1.4744292354e-04, -4.1197380770e-06, 1.8716778315e-04, 5.9466525950e-05, -2.1535406059e-04, -1.3428906721e-04, 2.2320253586e-04, 2.2381362224e-04, -2.0268101667e-04, -3.1959276378e-04, 1.4808186638e-04, 4.1001065917e-04, -5.7554211330e-05, -4.8146836586e-04, -6.5669953229e-05, 5.2020061780e-04, 2.1264356986e-04, -5.1456888354e-04, -3.6897010817e-04, 4.5742308855e-04, 5.1592368900e-04, -3.4844419302e-04, -6.3281999147e-04, 1.9563084815e-04, 7.0012094259e-04, -1.5797741756e-05, -7.0313729335e-04, -1.6636058573e-04, 6.3578234523e-04, 3.2080119242e-04, -5.0376229495e-04, -4.1621500961e-04, 3.2653836651e-04, 4.2535068081e-04, -1.3745364213e-04, -3.3101327728e-04, -1.8430456934e-05, 1.3194009206e-04, 8.8982755661e-05, 1.5239169201e-04, -2.2003824192e-05, -4.7906293052e-04, -2.2613704238e-04, 7.8152041882e-04, 6.8027040320e-04, -9.7374726658e-04, -1.3373013508e-03, 9.5741270435e-04, 2.1580289547e-03, -6.3299719228e-04, -3.0620992720e-03, -8.6269031694e-05, 3.9271120378e-03, 1.2587789534e-03, -4.5927565755e-03, -2.8987838754e-03, 4.8703372303e-03, 4.9622359304e-03, -4.5574398816e-03, -7.3359618474e-03, 3.4568288117e-03, 9.8315838933e-03, -1.3980578418e-03, -1.2184983600e-02, -1.7401747531e-03, 1.4060920812e-02, 6.0079548560e-03, -1.5064116199e-02, -1.1369040517e-02, 1.4746929342e-02, 1.7685514594e-02, -1.2617303866e-02, -2.4710772316e-02, 8.1301161027e-03, 3.2083363834e-02, -6.4508887676e-04, -3.9313241873e-02, -1.0691372050e-02, 4.5732214823e-02, 2.7247531405e-02, -5.0316598089e-02, -5.1710831086e-02, 5.1014637780e-02, 9.0562125115e-02, -4.1604553389e-02, -1.6373208262e-01, -1.0798078959e-02, 3.5635746743e-01, 5.5476789148e-01, 3.5635746743e-01, -1.0798078959e-02, -1.6373208262e-01, -4.1604553389e-02, 9.0562125115e-02, 5.1014637780e-02, -5.1710831086e-02, -5.0316598089e-02, 2.7247531405e-02, 4.5732214823e-02, -1.0691372050e-02, -3.9313241873e-02, -6.4508887676e-04, 3.2083363834e-02, 8.1301161027e-03, -2.4710772316e-02, -1.2617303866e-02, 1.7685514594e-02, 1.4746929342e-02, -1.1369040517e-02, -1.5064116199e-02, 6.0079548560e-03, 1.4060920812e-02, -1.7401747531e-03, -1.2184983600e-02, -1.3980578418e-03, 9.8315838933e-03, 3.4568288117e-03, -7.3359618474e-03, -4.5574398816e-03, 4.9622359304e-03, 4.8703372303e-03, -2.8987838754e-03, -4.5927565755e-03, 1.2587789534e-03, 3.9271120378e-03, -8.6269031694e-05, -3.0620992720e-03, -6.3299719228e-04, 2.1580289547e-03, 9.5741270435e-04, -1.3373013508e-03, -9.7374726658e-04, 6.8027040320e-04, 7.8152041882e-04, -2.2613704238e-04, -4.7906293052e-04, -2.2003824192e-05, 1.5239169201e-04, 8.8982755661e-05, 1.3194009206e-04, -1.8430456934e-05, -3.3101327728e-04, -1.3745364213e-04, 4.2535068081e-04, 3.2653836651e-04, -4.1621500961e-04, -5.0376229495e-04, 3.2080119242e-04, 6.3578234523e-04, -1.6636058573e-04, -7.0313729335e-04, -1.5797741756e-05, 7.0012094259e-04, 1.9563084815e-04, -6.3281999147e-04, -3.4844419302e-04, 5.1592368900e-04, 4.5742308855e-04, -3.6897010817e-04, -5.1456888354e-04, 2.1264356986e-04, 5.2020061780e-04, -6.5669953229e-05, -4.8146836586e-04, -5.7554211330e-05, 4.1001065917e-04, 1.4808186638e-04, -3.1959276378e-04, -2.0268101667e-04, 2.2381362224e-04, 2.2320253586e-04, -1.3428906721e-04, -2.1535406059e-04, 5.9466525950e-05, 1.8716778315e-04, -4.1197380770e-06, -1.4744292354e-04, -3.0537437275e-05, 1.0439175120e-04, 4.6255508155e-05, -6.4646397163e-05, -4.6855979020e-05, 3.2697649361e-05, 3.7229777761e-05, -1.0761588005e-05, -2.2400853889e-05, -1.0052830942e-06, 6.7569489549e-06, 4.0074018945e-06, 6.4151241761e-06, -6.3448606907e-07, -1.5160998822e-05, -6.3570800745e-06, 1.8817826302e-05, 1.4365010410e-05, -1.7788292260e-05, -2.1314973380e-05, 1.3203036531e-05, 2.5861850176e-05, -6.5499888643e-06, -2.7426623195e-05, -6.6351181774e-07, 2.6100740650e-05, 7.1690491210e-06, -2.2464707024e-05, -1.2093181242e-05, 1.7370299240e-05, 1.5009560931e-05, -1.1731346065e-05, -1.5904152941e-05, 6.3540333125e-06, 1.5076697696e-05, -1.8319006549e-06, -1.3019605998e-05, -1.5036743791e-06, 1.0288534956e-05, 3.5718171133e-06, -7.3975295122e-06, -4.4924073693e-06, 4.7473659764e-06, 4.5149379235e-06, -2.5916621140e-06, -3.9439819633e-06, 1.0369221285e-06, 3.0751670252e-06, -6.7734441506e-08, -2.1503018057e-06, -4.1410064947e-07, 1.3344925862e-06, 5.4727096517e-07, -7.1342595428e-07, -4.7412677515e-07, 3.0559714131e-07, 3.1450503208e-07, -8.3038042570e-08, -1.5226511473e-07, -6.2984765306e-09, 3.2745385737e-08, 1.8625056624e-08, 3.0931165073e-08, -7.9303729227e-10, -4.7317815530e-08, -1.6476043851e-08}
  COEFFICIENT_WIDTH 24
  QUANTIZATION Maximize_Dynamic_Range
  BESTPRECISION true
  FILTER_TYPE Decimation
  DECIMATION_RATE 2
  NUMBER_CHANNELS 2
  NUMBER_PATHS 1
  SAMPLE_FREQUENCY 1.0
  CLOCK_FREQUENCY 125
  OUTPUT_ROUNDING_MODE Convergent_Rounding_to_Even
  OUTPUT_WIDTH 25
  HAS_ARESETN true
} {
  S_AXIS_DATA conv_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter:1.1 subset_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 3
  TDATA_REMAP {tdata[23:0]}
} {
  S_AXIS fir_0/M_AXIS_DATA
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_constant
cell pavel-demin:user:axis_constant:1.0 cfg_0 {
  AXIS_TDATA_WIDTH 32
} {
  cfg_data const_0/dout
  aclk /pll_0/clk_out1
}

# Create axis_packetizer
cell pavel-demin:user:axis_packetizer:1.0 pktzr_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 1
  CONTINUOUS FALSE
} {
  S_AXIS cfg_0/M_AXIS
  cfg_data const_0/dout
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen:8.4 bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  USE_BYTE_WRITE_ENABLE true
  BYTE_SIZE 8
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 512
  WRITE_WIDTH_B 32
  WRITE_DEPTH_B 512
  ENABLE_A Always_Enabled
  ENABLE_B Always_Enabled
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
}

# Create axi_bram_writer
cell pavel-demin:user:axi_bram_writer:1.0 writer_0 {
  AXI_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 9
} {
  BRAM_PORTA bram_0/BRAM_PORTB
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_1 {
  CONST_WIDTH 9
  CONST_VAL 511
}

# Create axis_bram_reader
cell pavel-demin:user:axis_bram_reader:1.0 reader_0 {
  AXIS_TDATA_WIDTH 24
  BRAM_DATA_WIDTH 24
  BRAM_ADDR_WIDTH 9
  CONTINUOUS TRUE
} {
  BRAM_PORTA bram_0/BRAM_PORTA
  cfg_data const_1/dout
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_lfsr
cell pavel-demin:user:axis_lfsr:1.0 lfsr_1 {} {
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create cmpy
cell xilinx.com:ip:cmpy:6.0 mult_2 {
  APORTWIDTH.VALUE_SRC USER
  BPORTWIDTH.VALUE_SRC USER
  APORTWIDTH 24
  BPORTWIDTH 24
  ROUNDMODE Random_Rounding
  OUTPUTWIDTH 26
} {
  S_AXIS_A subset_0/M_AXIS
  S_AXIS_B reader_0/M_AXIS
  S_AXIS_CTRL lfsr_1/M_AXIS
  aclk /pll_0/clk_out1
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter:1.1 subset_1 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 3
  TDATA_REMAP {tdata[23:0]}
} {
  S_AXIS mult_2/M_AXIS_DOUT
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create xfft
cell xilinx.com:ip:xfft:9.1 fft_0 {
  INPUT_WIDTH.VALUE_SRC USER
  TRANSFORM_LENGTH 512
  TARGET_CLOCK_FREQUENCY 125
  TARGET_DATA_THROUGHPUT 1
  IMPLEMENTATION_OPTIONS radix_2_lite_burst_io
  RUN_TIME_CONFIGURABLE_TRANSFORM_LENGTH false
  INPUT_WIDTH 24
  PHASE_FACTOR_WIDTH 24
  SCALING_OPTIONS scaled
  ROUNDING_MODES convergent_rounding
  OUTPUT_ORDERING natural_order
  ARESETN true
} {
  S_AXIS_DATA subset_1/M_AXIS
  S_AXIS_CONFIG pktzr_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_dwidth_converter
cell xilinx.com:ip:axis_dwidth_converter:1.1 conv_1 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 3
  M_TDATA_NUM_BYTES 6
} {
  S_AXIS fft_0/M_AXIS_DATA
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster:1.1 bcast_1 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 6
  M_TDATA_NUM_BYTES 6
  M00_TDATA_REMAP {tdata[47:24],tdata[23:0]}
  M01_TDATA_REMAP {tdata[23:0],tdata[47:24]}
} {
  S_AXIS conv_1/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_lfsr
cell pavel-demin:user:axis_lfsr:1.0 lfsr_2 {} {
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create cmpy
cell xilinx.com:ip:cmpy:6.0 mult_3 {
  APORTWIDTH.VALUE_SRC USER
  BPORTWIDTH.VALUE_SRC USER
  APORTWIDTH 24
  BPORTWIDTH 24
  ROUNDMODE Random_Rounding
  OUTPUTWIDTH 26
} {
  S_AXIS_A bcast_1/M00_AXIS
  S_AXIS_B bcast_1/M01_AXIS
  S_AXIS_CTRL lfsr_2/M_AXIS
  aclk /pll_0/clk_out1
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter:1.1 subset_2 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 8
  M_TDATA_NUM_BYTES 3
  TDATA_REMAP {tdata[23:0]}
} {
  S_AXIS mult_3/M_AXIS_DOUT
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create floating_point
cell xilinx.com:ip:floating_point:7.1 fp_0 {
  OPERATION_TYPE Fixed_to_float
  A_PRECISION_TYPE.VALUE_SRC USER
  C_A_EXPONENT_WIDTH.VALUE_SRC USER
  C_A_FRACTION_WIDTH.VALUE_SRC USER
  A_PRECISION_TYPE Custom
  C_A_EXPONENT_WIDTH 2
  C_A_FRACTION_WIDTH 22
  RESULT_PRECISION_TYPE Single
  HAS_ARESETN true
} {
  S_AXIS_A subset_2/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create floating_point
cell xilinx.com:ip:floating_point:7.1 fp_1 {
  OPERATION_TYPE Logarithm
  RESULT_PRECISION_TYPE Single
  C_MULT_USAGE No_Usage
  HAS_ARESETN true
} {
  S_AXIS_A fp_0/M_AXIS_RESULT
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create floating_point
cell xilinx.com:ip:floating_point:7.1 fp_2 {
  OPERATION_TYPE Float_to_fixed
  RESULT_PRECISION_TYPE Custom
  C_RESULT_EXPONENT_WIDTH 22
  C_RESULT_FRACTION_WIDTH 10
  HAS_ARESETN true
} {
  S_AXIS_A fp_1/M_AXIS_RESULT
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create fifo_generator
cell xilinx.com:ip:fifo_generator:13.2 fifo_generator_0 {
  PERFORMANCE_OPTIONS First_Word_Fall_Through
  INPUT_DATA_WIDTH 32
  INPUT_DEPTH 1024
  OUTPUT_DATA_WIDTH 32
  OUTPUT_DEPTH 1024
  DATA_COUNT true
  DATA_COUNT_WIDTH 11
} {
  clk /pll_0/clk_out1
  srst slice_1/dout
}

# Create axis_fifo
cell pavel-demin:user:axis_averager:1.0 avgr_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 16
  AXIS_TDATA_SIGNED TRUE
} {
  S_AXIS fp_2/M_AXIS_RESULT
  FIFO_READ fifo_generator_0/FIFO_READ
  FIFO_WRITE fifo_generator_0/FIFO_WRITE
  pre_data slice_4/dout
  tot_data slice_5/dout
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create fifo_generator
cell xilinx.com:ip:fifo_generator:13.2 fifo_generator_1 {
  PERFORMANCE_OPTIONS First_Word_Fall_Through
  INPUT_DATA_WIDTH 32
  INPUT_DEPTH 1024
  OUTPUT_DATA_WIDTH 32
  OUTPUT_DEPTH 1024
  DATA_COUNT true
  DATA_COUNT_WIDTH 11
} {
  clk /pll_0/clk_out1
  srst slice_1/dout
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo:1.0 fifo_0 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
} {
  S_AXIS avgr_0/M_AXIS
  FIFO_READ fifo_generator_1/FIFO_READ
  FIFO_WRITE fifo_generator_1/FIFO_WRITE
  aclk /pll_0/clk_out1
}

# Create axi_axis_reader
cell pavel-demin:user:axi_axis_reader:1.0 reader_1 {
  AXI_DATA_WIDTH 32
} {
  S_AXIS fifo_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}
