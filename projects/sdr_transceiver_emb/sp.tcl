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
  DDS_CLOCK_RATE 122.88
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
    DIN_WIDTH 32 DIN_FROM 15 DIN_TO 0
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
    B_WIDTH 16
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
    MINIMUM_RATE 80
    MAXIMUM_RATE 5120
    FIXED_OR_INITIAL_RATE 80
    INPUT_SAMPLE_FREQUENCY 122.88
    CLOCK_FREQUENCY 122.88
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
  COEFFICIENTVECTOR {-1.6475807857e-08, -4.7315246749e-08, -7.9191187123e-10, 3.0929524568e-08, 1.8622389060e-08, 3.2743516899e-08, -6.2956586953e-09, -1.5225664922e-07, -8.3037205944e-08, 3.1448747856e-07, 3.0558593614e-07, -4.7409988167e-07, -7.1339494650e-07, 5.4723869856e-07, 1.3344308447e-06, -4.1407303088e-07, -2.1501992590e-06, -6.7740183845e-08, 3.0750178778e-06, 1.0368817458e-06, -3.9437887360e-06, -2.5915464085e-06, 4.5147153469e-06, 4.7471450381e-06, -4.4921850947e-06, -7.3971789503e-06, 3.5716401128e-06, 1.0288043408e-05, -1.5036000553e-06, -1.3018982613e-05, -1.8318091807e-06, 1.5075977860e-05, 6.3537175507e-06, -1.5903400302e-05, -1.1730765056e-05, 1.5008864024e-05, 1.7369443085e-05, -1.2092643302e-05, -2.2463607623e-05, 7.1687718975e-06, 2.6099477186e-05, -6.6357421347e-07, -2.7425318811e-05, -6.5495539247e-06, 2.5860657838e-05, 1.3202262513e-05, -2.1314050515e-05, -1.7787291359e-05, 1.4364484199e-05, 1.8816788458e-05, -6.3570078302e-06, -1.5160173328e-05, -6.3415461615e-07, 6.4147822095e-06, 4.0068519671e-06, 6.7565691908e-06, -1.0048430212e-06, -2.2399613709e-05, -1.0761465507e-05, 3.7227706671e-05, 3.2696438074e-05, -4.6853330595e-05, -6.4643576520e-05, 4.6252793415e-05, 1.0438691217e-04, -3.0535417801e-05, -1.4743588520e-04, -4.1201051593e-06, 1.8715870210e-04, 5.9464194280e-05, -2.1534351140e-04, -1.3428305319e-04, 2.2319154034e-04, 2.2380318735e-04, -2.0267100344e-04, -3.1957760239e-04, 1.4807455047e-04, 4.0999105993e-04, -5.7551395098e-05, -4.8144531166e-04, -6.5666641945e-05, 5.2017579159e-04, 2.1263297109e-04, -5.1454455577e-04, -3.6895180723e-04, 4.5740188692e-04, 5.1589824112e-04, -3.4842874558e-04, -6.3278901839e-04, 1.9562335374e-04, 7.0008706996e-04, -1.5799515581e-05, -7.0310389782e-04, -1.6634951777e-04, 6.3575310821e-04, 3.2078236839e-04, -5.0374059238e-04, -4.1619159520e-04, 3.2652655837e-04, 4.2532725852e-04, -1.3745235638e-04, -3.3099532668e-04, -1.8422927621e-05, 1.3193316841e-04, 8.8970816252e-05, 1.5238302379e-04, -2.1994580291e-05, -4.7903629852e-04, -2.2613422017e-04, 7.8147686875e-04, 6.8024486383e-04, -9.7369224215e-04, -1.3372425928e-03, 9.5735666932e-04, 2.1579284792e-03, -6.3295567552e-04, -3.0619526912e-03, -8.6276114956e-05, 3.9269212207e-03, 1.2587287894e-03, -4.5925315686e-03, -2.8986530550e-03, 4.8700976711e-03, 4.9620034841e-03, -4.5572156349e-03, -7.3356128401e-03, 3.4566596037e-03, 9.8311133330e-03, -1.3979917405e-03, -1.2184400383e-02, -1.7400840863e-03, 1.4060251431e-02, 6.0076521880e-03, -1.5063407785e-02, -1.1368473889e-02, 1.4746252447e-02, 1.7684641394e-02, -1.2616754701e-02, -2.4709566259e-02, 8.1298190817e-03, 3.2081823418e-02, -6.4520092280e-04, -3.9311402089e-02, -1.0690652393e-02, 4.5730167490e-02, 2.7245943769e-02, -5.0314537643e-02, -5.1708009336e-02, 5.1012992606e-02, 9.0557515184e-02, -4.1604467266e-02, -1.6372501093e-01, -1.0792454511e-02, 3.5635323199e-01, 5.5475775913e-01, 3.5635323199e-01, -1.0792454511e-02, -1.6372501093e-01, -4.1604467266e-02, 9.0557515184e-02, 5.1012992606e-02, -5.1708009336e-02, -5.0314537643e-02, 2.7245943769e-02, 4.5730167490e-02, -1.0690652393e-02, -3.9311402089e-02, -6.4520092280e-04, 3.2081823418e-02, 8.1298190817e-03, -2.4709566259e-02, -1.2616754701e-02, 1.7684641394e-02, 1.4746252447e-02, -1.1368473889e-02, -1.5063407785e-02, 6.0076521880e-03, 1.4060251431e-02, -1.7400840863e-03, -1.2184400383e-02, -1.3979917405e-03, 9.8311133330e-03, 3.4566596037e-03, -7.3356128401e-03, -4.5572156349e-03, 4.9620034841e-03, 4.8700976711e-03, -2.8986530550e-03, -4.5925315686e-03, 1.2587287894e-03, 3.9269212207e-03, -8.6276114956e-05, -3.0619526912e-03, -6.3295567552e-04, 2.1579284792e-03, 9.5735666932e-04, -1.3372425928e-03, -9.7369224215e-04, 6.8024486383e-04, 7.8147686875e-04, -2.2613422017e-04, -4.7903629852e-04, -2.1994580291e-05, 1.5238302379e-04, 8.8970816252e-05, 1.3193316841e-04, -1.8422927621e-05, -3.3099532668e-04, -1.3745235638e-04, 4.2532725852e-04, 3.2652655837e-04, -4.1619159520e-04, -5.0374059238e-04, 3.2078236839e-04, 6.3575310821e-04, -1.6634951777e-04, -7.0310389782e-04, -1.5799515581e-05, 7.0008706996e-04, 1.9562335374e-04, -6.3278901839e-04, -3.4842874558e-04, 5.1589824112e-04, 4.5740188692e-04, -3.6895180723e-04, -5.1454455577e-04, 2.1263297109e-04, 5.2017579159e-04, -6.5666641945e-05, -4.8144531166e-04, -5.7551395098e-05, 4.0999105993e-04, 1.4807455047e-04, -3.1957760239e-04, -2.0267100344e-04, 2.2380318735e-04, 2.2319154034e-04, -1.3428305319e-04, -2.1534351140e-04, 5.9464194280e-05, 1.8715870210e-04, -4.1201051593e-06, -1.4743588520e-04, -3.0535417801e-05, 1.0438691217e-04, 4.6252793415e-05, -6.4643576520e-05, -4.6853330595e-05, 3.2696438074e-05, 3.7227706671e-05, -1.0761465507e-05, -2.2399613709e-05, -1.0048430212e-06, 6.7565691908e-06, 4.0068519671e-06, 6.4147822095e-06, -6.3415461615e-07, -1.5160173328e-05, -6.3570078302e-06, 1.8816788458e-05, 1.4364484199e-05, -1.7787291359e-05, -2.1314050515e-05, 1.3202262513e-05, 2.5860657838e-05, -6.5495539247e-06, -2.7425318811e-05, -6.6357421347e-07, 2.6099477186e-05, 7.1687718975e-06, -2.2463607623e-05, -1.2092643302e-05, 1.7369443085e-05, 1.5008864024e-05, -1.1730765056e-05, -1.5903400302e-05, 6.3537175507e-06, 1.5075977860e-05, -1.8318091807e-06, -1.3018982613e-05, -1.5036000553e-06, 1.0288043408e-05, 3.5716401128e-06, -7.3971789503e-06, -4.4921850947e-06, 4.7471450381e-06, 4.5147153469e-06, -2.5915464085e-06, -3.9437887360e-06, 1.0368817458e-06, 3.0750178778e-06, -6.7740183845e-08, -2.1501992590e-06, -4.1407303088e-07, 1.3344308447e-06, 5.4723869856e-07, -7.1339494650e-07, -4.7409988167e-07, 3.0558593614e-07, 3.1448747856e-07, -8.3037205944e-08, -1.5225664922e-07, -6.2956586953e-09, 3.2743516899e-08, 1.8622389060e-08, 3.0929524568e-08, -7.9191187123e-10, -4.7315246749e-08, -1.6475807857e-08}
  COEFFICIENT_WIDTH 24
  QUANTIZATION Quantize_Only
  BESTPRECISION true
  FILTER_TYPE Decimation
  DECIMATION_RATE 2
  NUMBER_CHANNELS 2
  NUMBER_PATHS 1
  SAMPLE_FREQUENCY 1.536
  CLOCK_FREQUENCY 122.88
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
  WRITE_DEPTH_B 512
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
  TARGET_CLOCK_FREQUENCY 122.88
  TARGET_DATA_THROUGHPUT 1.536
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
