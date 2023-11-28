# Create clk_wiz
cell xilinx.com:ip:clk_wiz pll_0 {
  PRIMITIVE PLL
  PRIM_IN_FREQ.VALUE_SRC USER
  PRIM_IN_FREQ 125.0
  PRIM_SOURCE Differential_clock_capable_pin
  CLKOUT1_USED true
  CLKOUT1_REQUESTED_OUT_FREQ 125.0
  CLKOUT2_USED true
  CLKOUT2_REQUESTED_OUT_FREQ 250.0
  CLKOUT2_REQUESTED_PHASE 157.5
  CLKOUT3_USED true
  CLKOUT3_REQUESTED_OUT_FREQ 250.0
  CLKOUT3_REQUESTED_PHASE 202.5
  USE_RESET false
} {
  clk_in1_p adc_clk_p_i
  clk_in1_n adc_clk_n_i
}

# Create processing_system7
cell xilinx.com:ip:processing_system7 ps_0 {
  PCW_IMPORT_BOARD_PRESET cfg/red_pitaya.xml
  PCW_USE_S_AXI_ACP 1
  PCW_USE_DEFAULT_ACP_USER_VAL 1
} {
  M_AXI_GP0_ACLK pll_0/clk_out1
  S_AXI_ACP_ACLK pll_0/clk_out1
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {
  make_external {FIXED_IO, DDR}
  Master Disable
  Slave Disable
} [get_bd_cells ps_0]

# Create xlconstant
cell xilinx.com:ip:xlconstant const_0

# Create proc_sys_reset
cell xilinx.com:ip:proc_sys_reset rst_0 {} {
  ext_reset_in const_0/dout
  dcm_locked pll_0/locked
  slowest_sync_clk pll_0/clk_out1
}

# ADC

# Create axis_red_pitaya_adc
cell pavel-demin:user:axis_red_pitaya_adc adc_0 {
  ADC_DATA_WIDTH 14
} {
  aclk pll_0/clk_out1
  adc_dat_a adc_dat_a_i
  adc_dat_b adc_dat_b_i
  adc_csn adc_csn_o
}

# DAC

# Create axis_red_pitaya_dac
cell pavel-demin:user:axis_red_pitaya_dac dac_0 {
  DAC_DATA_WIDTH 14
} {
  aclk pll_0/clk_out1
  ddr_clk pll_0/clk_out2
  wrt_clk pll_0/clk_out3
  locked pll_0/locked
  dac_clk dac_clk_o
  dac_rst dac_rst_o
  dac_sel dac_sel_o
  dac_wrt dac_wrt_o
  dac_dat dac_dat_o
  s_axis_tvalid const_0/dout
}

# HUB

# Create axi_hub
cell pavel-demin:user:axi_hub hub_0 {
  CFG_DATA_WIDTH 224
  STS_DATA_WIDTH 32
} {
  S_AXI ps_0/M_AXI_GP0
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 224 DIN_FROM 0 DIN_TO 0
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 224 DIN_FROM 1 DIN_TO 1
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 224 DIN_FROM 31 DIN_TO 16
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 224 DIN_FROM 63 DIN_TO 32
} {
  din hub_0/cfg_data
}

# DDS

for {set i 0} {$i <= 3} {incr i} {

  # Create port_slicer
  cell pavel-demin:user:port_slicer slice_[expr $i + 4] {
    DIN_WIDTH 224 DIN_FROM [expr 32 * $i + 95] DIN_TO [expr 32 * $i + 64]
  } {
    din hub_0/cfg_data
  }

  # Create axis_constant
  cell pavel-demin:user:axis_constant phase_$i {
    AXIS_TDATA_WIDTH 32
  } {
    cfg_data slice_[expr $i + 4]/dout
    aclk pll_0/clk_out1
  }

  # Create dds_compiler
  cell xilinx.com:ip:dds_compiler dds_$i {
    DDS_CLOCK_RATE 125.0
    SPURIOUS_FREE_DYNAMIC_RANGE 138
    FREQUENCY_RESOLUTION 0.2
    PHASE_INCREMENT Streaming
    HAS_PHASE_OUT false
    PHASE_WIDTH 30
    OUTPUT_WIDTH 24
    DSP48_USE Minimal
    NEGATIVE_SINE true
  } {
    S_AXIS_PHASE phase_$i/M_AXIS
    aclk pll_0/clk_out1
  }

}

# RX

for {set i 0} {$i <= 3} {incr i} {

  # Create port_slicer
  cell pavel-demin:user:port_slicer adc_slice_$i {
    DIN_WIDTH 32 DIN_FROM [expr 16 * ($i / 2) + 15] DIN_TO [expr 16 * ($i / 2)]
  } {
    din adc_0/m_axis_tdata
  }

  # Create port_slicer
  cell pavel-demin:user:port_slicer dds_slice_$i {
    DIN_WIDTH 48 DIN_FROM [expr 24 * ($i % 2) + 23] DIN_TO [expr 24 * ($i % 2)]
  } {
    din dds_[expr $i / 2]/m_axis_data_tdata
  }

  # Create dsp48
  cell pavel-demin:user:dsp48 mult_$i {
    A_WIDTH 24
    B_WIDTH 14
    P_WIDTH 24
  } {
    A dds_slice_$i/dout
    B adc_slice_$i/dout
    CLK pll_0/clk_out1
  }

  # Create axis_variable
  cell pavel-demin:user:axis_variable rate_$i {
    AXIS_TDATA_WIDTH 16
  } {
    cfg_data slice_2/dout
    aclk pll_0/clk_out1
    aresetn slice_0/dout
  }

  # Create cic_compiler
  cell xilinx.com:ip:cic_compiler cic_$i {
    INPUT_DATA_WIDTH.VALUE_SRC USER
    FILTER_TYPE Decimation
    NUMBER_OF_STAGES 6
    SAMPLE_RATE_CHANGES Programmable
    MINIMUM_RATE 16
    MAXIMUM_RATE 8192
    FIXED_OR_INITIAL_RATE 16
    INPUT_SAMPLE_FREQUENCY 125.0
    CLOCK_FREQUENCY 125.0
    INPUT_DATA_WIDTH 24
    QUANTIZATION Truncation
    OUTPUT_DATA_WIDTH 32
    USE_XTREME_DSP_SLICE false
    HAS_ARESETN true
  } {
    s_axis_data_tdata mult_$i/P
    s_axis_data_tvalid const_0/dout
    S_AXIS_CONFIG rate_$i/M_AXIS
    aclk pll_0/clk_out1
    aresetn slice_0/dout
  }

}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
  NUM_SI 4
} {
  S00_AXIS cic_0/M_AXIS_DATA
  S01_AXIS cic_1/M_AXIS_DATA
  S02_AXIS cic_2/M_AXIS_DATA
  S03_AXIS cic_3/M_AXIS_DATA
  aclk pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_dwidth_converter
cell xilinx.com:ip:axis_dwidth_converter conv_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 16
  M_TDATA_NUM_BYTES 4
} {
  S_AXIS comb_0/M_AXIS
  aclk pll_0/clk_out1
  aresetn slice_0/dout
}

# Create fir_compiler
cell xilinx.com:ip:fir_compiler fir_0 {
  DATA_WIDTH.VALUE_SRC USER
  DATA_WIDTH 32
  COEFFICIENTVECTOR {-1.6466205515e-08, -4.7210930498e-08, -7.4624037568e-10, 3.0862903222e-08, 1.8514108516e-08, 3.2667629055e-08, -6.1812982861e-09, -1.5191287986e-07, -8.3003129347e-08, 3.1377466160e-07, 3.0513075584e-07, -4.7300779605e-07, -7.1213553661e-07, 5.4592845318e-07, 1.3319232858e-06, -4.1295160768e-07, -2.1460345369e-06, -6.7973116282e-08, 3.0689606447e-06, 1.0352414036e-06, -3.9359413665e-06, -2.5868470279e-06, 4.5056760794e-06, 4.7381719180e-06, -4.4831581086e-06, -7.3829415382e-06, 3.5644517882e-06, 1.0268080202e-05, -1.5005815946e-06, -1.2993665178e-05, -1.8280943151e-06, 1.5046743204e-05, 6.3408939805e-06, -1.5872833173e-05, -1.1707169275e-05, 1.4980559875e-05, 1.7334672982e-05, -1.2070794654e-05, -2.2418958581e-05, 7.1575108574e-06, 2.6048164708e-05, -6.6610517641e-07, -2.7372343672e-05, -6.5318937402e-06, 2.5812232020e-05, 1.3170832012e-05, -2.1276567049e-05, -1.7746646635e-05, 1.4343107896e-05, 1.8774642886e-05, -6.3540661835e-06, -1.5126650635e-05, -6.2070311049e-07, 6.4008947529e-06, 3.9845292965e-06, 6.7411480820e-06, -9.8698276876e-07, -2.2349252241e-05, -1.0756478318e-05, 3.7143603358e-05, 3.2647233472e-05, -4.6745783594e-05, -6.4529014109e-05, 4.6142555973e-05, 1.0419038116e-04, -3.0453418607e-05, -1.4715003818e-04, -4.1349961534e-06, 1.8678989929e-04, 5.9369483171e-05, -2.1491508680e-04, -1.3403879473e-04, 2.2274499047e-04, 2.2337938926e-04, -2.0226434662e-04, -3.1896185200e-04, 1.4777743617e-04, 4.0919507882e-04, -5.7437019616e-05, -4.8050901540e-04, -6.5532169138e-05, 5.1916752489e-04, 2.1220254089e-04, -5.1355652363e-04, -3.6820857632e-04, 4.5654080264e-04, 5.1486475524e-04, -3.4780133742e-04, -6.3153113417e-04, 1.9531891969e-04, 6.9871141442e-04, -1.5871470423e-05, -7.0174759671e-04, -1.6590011885e-04, 6.3456566412e-04, 3.2001798247e-04, -5.0285910379e-04, -4.1524077840e-04, 3.2604686516e-04, 4.2437610372e-04, -1.3739996163e-04, -3.3026635831e-04, -1.8117357564e-05, 1.3165198435e-04, 8.8486167503e-05, 1.5203104331e-04, -2.1619412890e-05, -4.7795483178e-04, -2.2601936249e-04, 7.7970838540e-04, 6.7920743695e-04, -9.7145781720e-04, -1.3348561280e-03, 9.5508124626e-04, 2.1538478261e-03, -6.3126989705e-04, -3.0559996572e-03, -8.6563469143e-05, 3.9191717081e-03, 1.2566912222e-03, -4.5833935652e-03, -2.8933398843e-03, 4.8603686625e-03, 4.9525630957e-03, -4.5481084590e-03, -7.3214386580e-03, 3.4497875870e-03, 9.8120025688e-03, -1.3953069848e-03, -1.2160714263e-02, -1.7364022846e-03, 1.4033065703e-02, 5.9953606649e-03, -1.5034636421e-02, -1.1345462396e-02, 1.4718760487e-02, 1.7649179118e-02, -1.2594449212e-02, -2.4660585291e-02, 8.1177525015e-03, 3.2019262053e-02, -6.4974611102e-04, -3.9236680163e-02, -1.0661432009e-02, 4.5647011829e-02, 2.7181472985e-02, -5.0230840944e-02, -5.1593414607e-02, 5.0946145943e-02, 9.0370279365e-02, -4.1600916843e-02, -1.6343771638e-01, -1.0563992168e-02, 3.5618113975e-01, 5.5434611620e-01, 3.5618113975e-01, -1.0563992168e-02, -1.6343771638e-01, -4.1600916843e-02, 9.0370279365e-02, 5.0946145943e-02, -5.1593414607e-02, -5.0230840944e-02, 2.7181472985e-02, 4.5647011829e-02, -1.0661432009e-02, -3.9236680163e-02, -6.4974611102e-04, 3.2019262053e-02, 8.1177525015e-03, -2.4660585291e-02, -1.2594449212e-02, 1.7649179118e-02, 1.4718760487e-02, -1.1345462396e-02, -1.5034636421e-02, 5.9953606649e-03, 1.4033065703e-02, -1.7364022846e-03, -1.2160714263e-02, -1.3953069848e-03, 9.8120025688e-03, 3.4497875870e-03, -7.3214386580e-03, -4.5481084590e-03, 4.9525630957e-03, 4.8603686625e-03, -2.8933398843e-03, -4.5833935652e-03, 1.2566912222e-03, 3.9191717081e-03, -8.6563469143e-05, -3.0559996572e-03, -6.3126989705e-04, 2.1538478261e-03, 9.5508124626e-04, -1.3348561280e-03, -9.7145781720e-04, 6.7920743695e-04, 7.7970838540e-04, -2.2601936249e-04, -4.7795483178e-04, -2.1619412890e-05, 1.5203104331e-04, 8.8486167503e-05, 1.3165198435e-04, -1.8117357564e-05, -3.3026635831e-04, -1.3739996163e-04, 4.2437610372e-04, 3.2604686516e-04, -4.1524077840e-04, -5.0285910379e-04, 3.2001798247e-04, 6.3456566412e-04, -1.6590011885e-04, -7.0174759671e-04, -1.5871470423e-05, 6.9871141442e-04, 1.9531891969e-04, -6.3153113417e-04, -3.4780133742e-04, 5.1486475524e-04, 4.5654080264e-04, -3.6820857632e-04, -5.1355652363e-04, 2.1220254089e-04, 5.1916752489e-04, -6.5532169138e-05, -4.8050901540e-04, -5.7437019616e-05, 4.0919507882e-04, 1.4777743617e-04, -3.1896185200e-04, -2.0226434662e-04, 2.2337938926e-04, 2.2274499047e-04, -1.3403879473e-04, -2.1491508680e-04, 5.9369483171e-05, 1.8678989929e-04, -4.1349961534e-06, -1.4715003818e-04, -3.0453418607e-05, 1.0419038116e-04, 4.6142555973e-05, -6.4529014109e-05, -4.6745783594e-05, 3.2647233472e-05, 3.7143603358e-05, -1.0756478318e-05, -2.2349252241e-05, -9.8698276876e-07, 6.7411480820e-06, 3.9845292965e-06, 6.4008947529e-06, -6.2070311049e-07, -1.5126650635e-05, -6.3540661835e-06, 1.8774642886e-05, 1.4343107896e-05, -1.7746646635e-05, -2.1276567049e-05, 1.3170832012e-05, 2.5812232020e-05, -6.5318937402e-06, -2.7372343672e-05, -6.6610517641e-07, 2.6048164708e-05, 7.1575108574e-06, -2.2418958581e-05, -1.2070794654e-05, 1.7334672982e-05, 1.4980559875e-05, -1.1707169275e-05, -1.5872833173e-05, 6.3408939805e-06, 1.5046743204e-05, -1.8280943151e-06, -1.2993665178e-05, -1.5005815946e-06, 1.0268080202e-05, 3.5644517882e-06, -7.3829415382e-06, -4.4831581086e-06, 4.7381719180e-06, 4.5056760794e-06, -2.5868470279e-06, -3.9359413665e-06, 1.0352414036e-06, 3.0689606447e-06, -6.7973116282e-08, -2.1460345369e-06, -4.1295160768e-07, 1.3319232858e-06, 5.4592845318e-07, -7.1213553661e-07, -4.7300779605e-07, 3.0513075584e-07, 3.1377466160e-07, -8.3003129347e-08, -1.5191287986e-07, -6.1812982861e-09, 3.2667629055e-08, 1.8514108516e-08, 3.0862903222e-08, -7.4624037568e-10, -4.7210930498e-08, -1.6466205515e-08}
  COEFFICIENT_WIDTH 24
  QUANTIZATION Quantize_Only
  BESTPRECISION true
  FILTER_TYPE Decimation
  DECIMATION_RATE 2
  NUMBER_CHANNELS 4
  NUMBER_PATHS 1
  SAMPLE_FREQUENCY 7.8125
  CLOCK_FREQUENCY 125.0
  OUTPUT_ROUNDING_MODE Convergent_Rounding_to_Even
  OUTPUT_WIDTH 26
  M_DATA_HAS_TREADY true
  HAS_ARESETN true
} {
  S_AXIS_DATA conv_0/M_AXIS
  aclk pll_0/clk_out1
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
  aclk pll_0/clk_out1
  aresetn slice_0/dout
}

# Create floating_point
cell xilinx.com:ip:floating_point fp_0 {
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
  S_AXIS_A subset_0/M_AXIS
  aclk pll_0/clk_out1
  aresetn slice_0/dout
}

# DMA

# Create xlconstant
cell xilinx.com:ip:xlconstant const_1 {
  CONST_WIDTH 16
  CONST_VAL 65535
}

# Create axis_ram_writer
cell pavel-demin:user:axis_ram_writer writer_0 {
  ADDR_WIDTH 16
  AXI_ID_WIDTH 3
  AXIS_TDATA_WIDTH 32
  FIFO_WRITE_DEPTH 1024
} {
  S_AXIS fp_0/M_AXIS_RESULT
  M_AXI ps_0/S_AXI_ACP
  min_addr slice_3/dout
  cfg_data const_1/dout
  sts_data hub_0/sts_data
  aclk pll_0/clk_out1
  aresetn slice_1/dout
}

# GEN

for {set i 0} {$i <= 1} {incr i} {

  # Create port_slicer
  cell pavel-demin:user:port_slicer slice_[expr $i + 8] {
    DIN_WIDTH 224 DIN_FROM [expr 16 * $i + 207] DIN_TO [expr 16 * $i + 192]
  } {
    din hub_0/cfg_data
  }

  # Create dsp48
  cell pavel-demin:user:dsp48 mult_[expr $i + 4] {
    A_WIDTH 24
    B_WIDTH 16
    P_WIDTH 14
  } {
    A dds_[expr $i + 2]/m_axis_data_tdata
    B slice_[expr $i + 8]/dout
    CLK pll_0/clk_out1
  }

}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  NUM_PORTS 2
  IN0_WIDTH 16
  IN1_WIDTH 16
} {
  In0 mult_4/P
  In1 mult_5/P
  dout dac_0/s_axis_tdata
}
