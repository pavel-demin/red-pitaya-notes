# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 8 DIN_FROM 1 DIN_TO 1
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 8 DIN_FROM 2 DIN_TO 2
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 128 DIN_FROM 15 DIN_TO 0
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 128 DIN_FROM 63 DIN_TO 32
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_4 {
  DIN_WIDTH 128 DIN_FROM 79 DIN_TO 64
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_5 {
  DIN_WIDTH 128 DIN_FROM 95 DIN_TO 80
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_6 {
  DIN_WIDTH 128 DIN_FROM 127 DIN_TO 96
}

# Create axis_constant
cell pavel-demin:user:axis_constant phase_0 {
  AXIS_TDATA_WIDTH 32
} {
  cfg_data slice_3/dout
  aclk /pll_0/clk_out1
}

# Create dds_compiler
cell xilinx.com:ip:dds_compiler dds_0 {
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
cell pavel-demin:user:axis_lfsr lfsr_0 {} {
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create xlconstant
cell xilinx.com:ip:xlconstant const_0

for {set i 0} {$i <= 1} {incr i} {

  # Create port_slicer
  cell pavel-demin:user:port_slicer adc_slice_$i {
    DIN_WIDTH 32 DIN_FROM 13 DIN_TO 0
  } {
    din /adc_0/m_axis_tdata
  }

  # Create port_slicer
  cell pavel-demin:user:port_slicer dds_slice_$i {
    DIN_WIDTH 48 DIN_FROM [expr 24 * $i + 23] DIN_TO [expr 24 * $i]
  } {
    din dds_0/m_axis_data_tdata
  }

  # Create xbip_dsp48_macro
  cell xilinx.com:ip:xbip_dsp48_macro mult_$i {
    INSTRUCTION1 RNDSIMPLE(A*B+CARRYIN)
    A_WIDTH.VALUE_SRC USER
    B_WIDTH.VALUE_SRC USER
    OUTPUT_PROPERTIES User_Defined
    A_WIDTH 24
    B_WIDTH 14
    P_WIDTH 25
  } {
    A dds_slice_$i/dout
    B adc_slice_$i/dout
    CARRYIN lfsr_0/m_axis_tdata
    CLK /pll_0/clk_out1
  }

  # Create axis_variable
  cell pavel-demin:user:axis_variable rate_$i {
    AXIS_TDATA_WIDTH 16
  } {
    cfg_data slice_2/dout
    aclk /pll_0/clk_out1
    aresetn slice_0/dout
  }

  # Create cic_compiler
  cell xilinx.com:ip:cic_compiler cic_$i {
    INPUT_DATA_WIDTH.VALUE_SRC USER
    FILTER_TYPE Decimation
    NUMBER_OF_STAGES 6
    SAMPLE_RATE_CHANGES Programmable
    MINIMUM_RATE 125
    MAXIMUM_RATE 6250
    FIXED_OR_INITIAL_RATE 125
    INPUT_SAMPLE_FREQUENCY 125
    CLOCK_FREQUENCY 125
    INPUT_DATA_WIDTH 24
    QUANTIZATION Truncation
    OUTPUT_DATA_WIDTH 32
    USE_XTREME_DSP_SLICE false
    HAS_ARESETN true
  } {
    s_axis_data_tdata mult_$i/P
    s_axis_data_tvalid const_0/dout
    S_AXIS_CONFIG rate_$i/M_AXIS
    aclk /pll_0/clk_out1
    aresetn slice_0/dout
  }

}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
} {
  S00_AXIS cic_0/M_AXIS_DATA
  S01_AXIS cic_1/M_AXIS_DATA
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_dwidth_converter
cell xilinx.com:ip:axis_dwidth_converter conv_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 8
  M_TDATA_NUM_BYTES 4
} {
  S_AXIS comb_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create fir_compiler
cell xilinx.com:ip:fir_compiler fir_0 {
  DATA_WIDTH.VALUE_SRC USER
  DATA_WIDTH 32
  COEFFICIENTVECTOR {-1.6476043851e-08, -4.7317815530e-08, -7.9303729227e-10, 3.0931165073e-08, 1.8625056624e-08, 3.2745385737e-08, -6.2984765306e-09, -1.5226511473e-07, -8.3038042570e-08, 3.1450503208e-07, 3.0559714131e-07, -4.7412677515e-07, -7.1342595428e-07, 5.4727096517e-07, 1.3344925862e-06, -4.1410064947e-07, -2.1503018057e-06, -6.7734441506e-08, 3.0751670252e-06, 1.0369221285e-06, -3.9439819633e-06, -2.5916621140e-06, 4.5149379235e-06, 4.7473659764e-06, -4.4924073693e-06, -7.3975295122e-06, 3.5718171133e-06, 1.0288534956e-05, -1.5036743791e-06, -1.3019605998e-05, -1.8319006549e-06, 1.5076697696e-05, 6.3540333125e-06, -1.5904152941e-05, -1.1731346065e-05, 1.5009560931e-05, 1.7370299240e-05, -1.2093181242e-05, -2.2464707024e-05, 7.1690491210e-06, 2.6100740650e-05, -6.6351181774e-07, -2.7426623195e-05, -6.5499888643e-06, 2.5861850176e-05, 1.3203036531e-05, -2.1314973380e-05, -1.7788292260e-05, 1.4365010410e-05, 1.8817826302e-05, -6.3570800745e-06, -1.5160998822e-05, -6.3448606907e-07, 6.4151241761e-06, 4.0074018945e-06, 6.7569489549e-06, -1.0052830942e-06, -2.2400853889e-05, -1.0761588005e-05, 3.7229777761e-05, 3.2697649361e-05, -4.6855979020e-05, -6.4646397163e-05, 4.6255508155e-05, 1.0439175120e-04, -3.0537437275e-05, -1.4744292354e-04, -4.1197380770e-06, 1.8716778315e-04, 5.9466525950e-05, -2.1535406059e-04, -1.3428906721e-04, 2.2320253586e-04, 2.2381362224e-04, -2.0268101667e-04, -3.1959276378e-04, 1.4808186638e-04, 4.1001065917e-04, -5.7554211330e-05, -4.8146836586e-04, -6.5669953229e-05, 5.2020061780e-04, 2.1264356986e-04, -5.1456888354e-04, -3.6897010817e-04, 4.5742308855e-04, 5.1592368900e-04, -3.4844419302e-04, -6.3281999147e-04, 1.9563084815e-04, 7.0012094259e-04, -1.5797741756e-05, -7.0313729335e-04, -1.6636058573e-04, 6.3578234523e-04, 3.2080119242e-04, -5.0376229495e-04, -4.1621500961e-04, 3.2653836651e-04, 4.2535068081e-04, -1.3745364213e-04, -3.3101327728e-04, -1.8430456934e-05, 1.3194009206e-04, 8.8982755661e-05, 1.5239169201e-04, -2.2003824192e-05, -4.7906293052e-04, -2.2613704238e-04, 7.8152041882e-04, 6.8027040320e-04, -9.7374726658e-04, -1.3373013508e-03, 9.5741270435e-04, 2.1580289547e-03, -6.3299719228e-04, -3.0620992720e-03, -8.6269031694e-05, 3.9271120378e-03, 1.2587789534e-03, -4.5927565755e-03, -2.8987838754e-03, 4.8703372303e-03, 4.9622359304e-03, -4.5574398816e-03, -7.3359618474e-03, 3.4568288117e-03, 9.8315838933e-03, -1.3980578418e-03, -1.2184983600e-02, -1.7401747531e-03, 1.4060920812e-02, 6.0079548560e-03, -1.5064116199e-02, -1.1369040517e-02, 1.4746929342e-02, 1.7685514594e-02, -1.2617303866e-02, -2.4710772316e-02, 8.1301161027e-03, 3.2083363834e-02, -6.4508887676e-04, -3.9313241873e-02, -1.0691372050e-02, 4.5732214823e-02, 2.7247531405e-02, -5.0316598089e-02, -5.1710831086e-02, 5.1014637780e-02, 9.0562125115e-02, -4.1604553389e-02, -1.6373208262e-01, -1.0798078959e-02, 3.5635746743e-01, 5.5476789148e-01, 3.5635746743e-01, -1.0798078959e-02, -1.6373208262e-01, -4.1604553389e-02, 9.0562125115e-02, 5.1014637780e-02, -5.1710831086e-02, -5.0316598089e-02, 2.7247531405e-02, 4.5732214823e-02, -1.0691372050e-02, -3.9313241873e-02, -6.4508887676e-04, 3.2083363834e-02, 8.1301161027e-03, -2.4710772316e-02, -1.2617303866e-02, 1.7685514594e-02, 1.4746929342e-02, -1.1369040517e-02, -1.5064116199e-02, 6.0079548560e-03, 1.4060920812e-02, -1.7401747531e-03, -1.2184983600e-02, -1.3980578418e-03, 9.8315838933e-03, 3.4568288117e-03, -7.3359618474e-03, -4.5574398816e-03, 4.9622359304e-03, 4.8703372303e-03, -2.8987838754e-03, -4.5927565755e-03, 1.2587789534e-03, 3.9271120378e-03, -8.6269031694e-05, -3.0620992720e-03, -6.3299719228e-04, 2.1580289547e-03, 9.5741270435e-04, -1.3373013508e-03, -9.7374726658e-04, 6.8027040320e-04, 7.8152041882e-04, -2.2613704238e-04, -4.7906293052e-04, -2.2003824192e-05, 1.5239169201e-04, 8.8982755661e-05, 1.3194009206e-04, -1.8430456934e-05, -3.3101327728e-04, -1.3745364213e-04, 4.2535068081e-04, 3.2653836651e-04, -4.1621500961e-04, -5.0376229495e-04, 3.2080119242e-04, 6.3578234523e-04, -1.6636058573e-04, -7.0313729335e-04, -1.5797741756e-05, 7.0012094259e-04, 1.9563084815e-04, -6.3281999147e-04, -3.4844419302e-04, 5.1592368900e-04, 4.5742308855e-04, -3.6897010817e-04, -5.1456888354e-04, 2.1264356986e-04, 5.2020061780e-04, -6.5669953229e-05, -4.8146836586e-04, -5.7554211330e-05, 4.1001065917e-04, 1.4808186638e-04, -3.1959276378e-04, -2.0268101667e-04, 2.2381362224e-04, 2.2320253586e-04, -1.3428906721e-04, -2.1535406059e-04, 5.9466525950e-05, 1.8716778315e-04, -4.1197380770e-06, -1.4744292354e-04, -3.0537437275e-05, 1.0439175120e-04, 4.6255508155e-05, -6.4646397163e-05, -4.6855979020e-05, 3.2697649361e-05, 3.7229777761e-05, -1.0761588005e-05, -2.2400853889e-05, -1.0052830942e-06, 6.7569489549e-06, 4.0074018945e-06, 6.4151241761e-06, -6.3448606907e-07, -1.5160998822e-05, -6.3570800745e-06, 1.8817826302e-05, 1.4365010410e-05, -1.7788292260e-05, -2.1314973380e-05, 1.3203036531e-05, 2.5861850176e-05, -6.5499888643e-06, -2.7426623195e-05, -6.6351181774e-07, 2.6100740650e-05, 7.1690491210e-06, -2.2464707024e-05, -1.2093181242e-05, 1.7370299240e-05, 1.5009560931e-05, -1.1731346065e-05, -1.5904152941e-05, 6.3540333125e-06, 1.5076697696e-05, -1.8319006549e-06, -1.3019605998e-05, -1.5036743791e-06, 1.0288534956e-05, 3.5718171133e-06, -7.3975295122e-06, -4.4924073693e-06, 4.7473659764e-06, 4.5149379235e-06, -2.5916621140e-06, -3.9439819633e-06, 1.0369221285e-06, 3.0751670252e-06, -6.7734441506e-08, -2.1503018057e-06, -4.1410064947e-07, 1.3344925862e-06, 5.4727096517e-07, -7.1342595428e-07, -4.7412677515e-07, 3.0559714131e-07, 3.1450503208e-07, -8.3038042570e-08, -1.5226511473e-07, -6.2984765306e-09, 3.2745385737e-08, 1.8625056624e-08, 3.0931165073e-08, -7.9303729227e-10, -4.7317815530e-08, -1.6476043851e-08}
  COEFFICIENT_WIDTH 24
  QUANTIZATION Quantize_Only
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
  aresetn slice_0/dout
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter subset_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 3
  TDATA_REMAP {tdata[23:0]}
} {
  S_AXIS fir_0/M_AXIS_DATA
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_dwidth_converter
cell xilinx.com:ip:axis_dwidth_converter conv_1 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 3
  M_TDATA_NUM_BYTES 6
} {
  S_AXIS subset_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_data_fifo
cell xilinx.com:ip:axis_data_fifo fifo_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 6
  FIFO_DEPTH 1024
} {
  S_AXIS conv_1/M_AXIS
  s_axis_aclk /pll_0/clk_out1
  s_axis_aresetn slice_0/dout
}

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  USE_BYTE_WRITE_ENABLE true
  BYTE_SIZE 8
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 512
  WRITE_WIDTH_B 32
  ENABLE_A Always_Enabled
  ENABLE_B Always_Enabled
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
}

# Create axi_bram_writer
cell pavel-demin:user:axi_bram_writer writer_0 {
  AXI_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 9
} {
  BRAM_PORTA bram_0/BRAM_PORTB
}

# Create xlconstant
cell xilinx.com:ip:xlconstant const_1 {
  CONST_WIDTH 9
  CONST_VAL 511
}

# Create axis_bram_reader
cell pavel-demin:user:axis_bram_reader reader_0 {
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
cell pavel-demin:user:axis_lfsr lfsr_1 {
  HAS_TREADY TRUE
} {
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create cmpy
cell xilinx.com:ip:cmpy mult_2 {
  FLOWCONTROL Blocking
  APORTWIDTH.VALUE_SRC USER
  BPORTWIDTH.VALUE_SRC USER
  APORTWIDTH 24
  BPORTWIDTH 24
  ROUNDMODE Random_Rounding
  OUTPUTWIDTH 26
} {
  S_AXIS_A fifo_0/M_AXIS
  S_AXIS_B reader_0/M_AXIS
  S_AXIS_CTRL lfsr_1/M_AXIS
  aclk /pll_0/clk_out1
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter subset_1 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 8
  M_TDATA_NUM_BYTES 6
  TDATA_REMAP {tdata[55:32],tdata[23:0]}
} {
  S_AXIS mult_2/M_AXIS_DOUT
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_variable
cell pavel-demin:user:axis_variable cfg_0 {
  AXIS_TDATA_WIDTH 24
} {
  cfg_data const_0/dout
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create xfft
cell xilinx.com:ip:xfft fft_0 {
  INPUT_WIDTH.VALUE_SRC USER
  TRANSFORM_LENGTH 512
  TARGET_CLOCK_FREQUENCY 125
  TARGET_DATA_THROUGHPUT 1
  IMPLEMENTATION_OPTIONS radix_2_lite_burst_io
  RUN_TIME_CONFIGURABLE_TRANSFORM_LENGTH false
  INPUT_WIDTH 24
  PHASE_FACTOR_WIDTH 24
  SCALING_OPTIONS unscaled
  ROUNDING_MODES convergent_rounding
  OUTPUT_ORDERING natural_order
  BUTTERFLY_TYPE use_xtremedsp_slices
  ARESETN true
} {
  S_AXIS_DATA subset_1/M_AXIS
  S_AXIS_CONFIG cfg_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_data_fifo
cell xilinx.com:ip:axis_data_fifo fifo_1 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 10
  FIFO_DEPTH 1024
} {
  S_AXIS fft_0/M_AXIS_DATA
  s_axis_aclk /pll_0/clk_out1
  s_axis_aresetn slice_0/dout
}

# Create cordic
cell xilinx.com:ip:cordic cordic_0 {
  INPUT_WIDTH.VALUE_SRC USER
  FUNCTIONAL_SELECTION Translate
  PIPELINING_MODE Optimal
  INPUT_WIDTH 34
  OUTPUT_WIDTH 32
  ROUND_MODE Round_Pos_Neg_Inf
  COMPENSATION_SCALING Embedded_Multiplier
  ARCHITECTURAL_CONFIGURATION Word_Serial
  ARESETN true
} {
  S_AXIS_CARTESIAN fifo_1/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter subset_2 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 8
  M_TDATA_NUM_BYTES 4
  TDATA_REMAP {tdata[31:0]}
} {
  S_AXIS cordic_0/M_AXIS_DOUT
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create fifo_generator
cell xilinx.com:ip:fifo_generator fifo_generator_0 {
  PERFORMANCE_OPTIONS First_Word_Fall_Through
  INPUT_DATA_WIDTH 40
  INPUT_DEPTH 1024
  OUTPUT_DATA_WIDTH 40
  OUTPUT_DEPTH 1024
  DATA_COUNT true
  DATA_COUNT_WIDTH 11
  PROGRAMMABLE_FULL_TYPE Single_Programmable_Full_Threshold_Constant
  FULL_THRESHOLD_ASSERT_VALUE 512
} {
  clk /pll_0/clk_out1
  srst slice_1/dout
}

# Create axis_fifo
cell pavel-demin:user:axis_averager avgr_0 {
  AXIS_TDATA_WIDTH 40
  CNTR_WIDTH 16
  AXIS_TDATA_SIGNED FALSE
} {
  S_AXIS subset_2/M_AXIS
  FIFO_READ fifo_generator_0/FIFO_READ
  FIFO_WRITE fifo_generator_0/FIFO_WRITE
  fifo_prog_full fifo_generator_0/prog_full
  pre_data const_1/dout
  tot_data slice_4/dout
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create mult_gen
cell xilinx.com:ip:mult_gen mult_4 {
  PORTATYPE.VALUE_SRC USER
  PORTAWIDTH.VALUE_SRC USER
  PORTBTYPE.VALUE_SRC USER
  PORTBWIDTH.VALUE_SRC USER
  PORTATYPE Unsigned
  PORTAWIDTH 40
  PORTBTYPE Unsigned
  PORTBWIDTH 16
  USE_CUSTOM_OUTPUT_WIDTH true
  OUTPUTWIDTHHIGH 47
  OUTPUTWIDTHLOW 16
  MULTIPLIER_CONSTRUCTION Use_Mults
  PIPESTAGES 4
} {
  A avgr_0/m_axis_tdata
  B slice_5/dout
  CLK /pll_0/clk_out1
}

# Create c_shift_ram
cell xilinx.com:ip:c_shift_ram delay_0 {
  WIDTH.VALUE_SRC USER
  WIDTH 1
  DEPTH 4
} {
  D avgr_0/m_axis_tvalid
  CLK /pll_0/clk_out1
}

# Create floating_point
cell xilinx.com:ip:floating_point fp_0 {
  OPERATION_TYPE Fixed_to_float
  A_PRECISION_TYPE.VALUE_SRC USER
  C_A_EXPONENT_WIDTH.VALUE_SRC USER
  C_A_FRACTION_WIDTH.VALUE_SRC USER
  A_PRECISION_TYPE Custom
  C_A_EXPONENT_WIDTH 2
  C_A_FRACTION_WIDTH 30
  RESULT_PRECISION_TYPE Single
  HAS_ARESETN true
} {
  s_axis_a_tdata mult_4/P
  s_axis_a_tvalid delay_0/Q
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create floating_point
cell xilinx.com:ip:floating_point fp_1 {
  OPERATION_TYPE Logarithm
  RESULT_PRECISION_TYPE Single
  HAS_ARESETN true
} {
  S_AXIS_A fp_0/M_AXIS_RESULT
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_variable
cell pavel-demin:user:axis_constant const_3 {
  AXIS_TDATA_WIDTH 32
} {
  cfg_data slice_6/dout
  aclk /pll_0/clk_out1
}

# Create floating_point
cell xilinx.com:ip:floating_point fp_2 {
  OPERATION_TYPE Multiply
  RESULT_PRECISION_TYPE Single
  HAS_ARESETN true
} {
  S_AXIS_A fp_1/M_AXIS_RESULT
  S_AXIS_B const_3/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create fifo_generator
cell xilinx.com:ip:fifo_generator fifo_generator_1 {
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
cell pavel-demin:user:axis_fifo fifo_2 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
} {
  S_AXIS fp_2/M_AXIS_RESULT
  FIFO_READ fifo_generator_1/FIFO_READ
  FIFO_WRITE fifo_generator_1/FIFO_WRITE
  aclk /pll_0/clk_out1
}

# Create axi_axis_reader
cell pavel-demin:user:axi_axis_reader reader_1 {
  AXI_DATA_WIDTH 32
} {
  S_AXIS fifo_2/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}
