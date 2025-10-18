# Create clk_wiz
cell xilinx.com:ip:clk_wiz pll_0 {
  PRIMITIVE PLL
  PRIM_IN_FREQ.VALUE_SRC USER
  PRIM_IN_FREQ 122.88
  PRIM_SOURCE Differential_clock_capable_pin
  CLKOUT1_USED true
  CLKOUT1_REQUESTED_OUT_FREQ 122.88
  CLKOUT2_USED true
  CLKOUT2_REQUESTED_OUT_FREQ 245.76
  CLKOUT2_REQUESTED_PHASE 157.5
  CLKOUT3_USED true
  CLKOUT3_REQUESTED_OUT_FREQ 245.76
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
  ADC_DATA_WIDTH 16
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

  # Create dds
  cell pavel-demin:user:dds dds_$i {
    NEGATIVE_SINE TRUE
  } {
    pinc slice_[expr $i + 4]/dout
    aclk pll_0/clk_out1
    aresetn rst_0/peripheral_aresetn
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
    din dds_[expr $i / 2]/dout
  }

  # Create dsp48
  cell pavel-demin:user:dsp48 mult_$i {
    A_WIDTH 24
    B_WIDTH 16
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
    MINIMUM_RATE 4
    MAXIMUM_RATE 64
    FIXED_OR_INITIAL_RATE 4
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

# Create fir_compiler
cell xilinx.com:ip:fir_compiler fir_0 {
  DATA_WIDTH.VALUE_SRC USER
  DATA_WIDTH 32
  COEFFICIENTVECTOR {-4.3206471974e-08, 1.9581108834e-08, 3.8109090962e-08, 1.4084131418e-09, 1.2186120186e-08, -4.3002365521e-08, -1.3853077565e-07, 1.0133745579e-07, 3.7888894052e-07, -1.6394613230e-07, -7.7661761489e-07, 2.0712149544e-07, 1.3774224210e-06, -1.9384365527e-07, -2.2249377290e-06, 7.2763377227e-08, 3.3549988889e-06, 2.2117927620e-07, -4.7889010594e-06, -7.6397724595e-07, 6.5260611238e-06, 1.6368965465e-06, -8.5375061091e-06, -2.9191987685e-06, 1.0758341788e-05, 4.6752496998e-06, -1.3084991486e-05, -6.9412383420e-06, 1.5373724961e-05, 9.7084647273e-06, -1.7443962288e-05, -1.2905490830e-05, 1.9086646276e-05, 1.6380162505e-05, -2.0078608383e-05, -1.9883422927e-05, 2.0203022903e-05, 2.3056727953e-05, -1.9275907764e-05, -2.5425425753e-05, 1.7177613167e-05, 2.6400053739e-05, -1.3887984508e-05, -2.5287767236e-05, 9.5223982251e-06, 2.1313177486e-05, -4.3728197481e-06, -1.3665196308e-05, -1.0766754409e-06, 1.5310917825e-06, 6.1157778312e-06, 1.5831781930e-05, -9.8065695691e-06, -3.9025740577e-05, 1.0994139826e-05, 6.8433254890e-05, -8.3394585095e-06, -1.0413520871e-04, 3.7650515294e-07, 1.4583183818e-04, 1.4403443484e-05, -1.9277411182e-04, -3.7440682291e-05, 2.4371243539e-04, 6.9969543240e-05, -2.9687173927e-04, -1.1285857228e-04, 3.4996062912e-04, 1.6643173499e-04, -4.0023458429e-04, -2.3031764274e-04, 4.4453252714e-04, 3.0318466104e-04, -4.7951215941e-04, -3.8264756455e-04, 5.0179308310e-04, 4.6508817914e-04, -5.0822117940e-04, -5.4554763871e-04, 4.9616951008e-04, 6.1766717590e-04, -4.6388070868e-04, -6.7369245331e-04, 4.1083915738e-04, 7.0454923950e-04, -3.3816268708e-04, -6.9999979578e-04, 2.4899722791e-04, 6.4888144112e-04, -1.4890118284e-04, -5.3942758146e-04, 4.6206061884e-05, 3.5965967563e-04, 4.7605082323e-05, -9.8021126546e-05, -1.1791553911e-04, -2.5647044402e-04, 1.4635687092e-04, 7.1327754530e-04, -1.1093829736e-04, -1.2799048805e-03, -1.4106495666e-05, 1.9613024094e-03, 2.5839388583e-04, -2.7593013177e-03, -6.5548048055e-04, 3.6720962046e-03, 1.2430372921e-03, -4.6937909542e-03, -2.0632159584e-03, 5.8140101675e-03, 3.1633610776e-03, -7.0175713933e-03, -4.5973156063e-03, 8.2841916600e-03, 6.4277320980e-03, -9.5881685667e-03, -8.7303353142e-03, 1.0896800128e-02, 1.1598799243e-02, -1.2171477770e-02, -1.5158439259e-02, 1.3362938210e-02, 1.9583979076e-02, -1.4406996028e-02, -2.5135038845e-02, 1.5213119554e-02, 3.2223155290e-02, -1.5638192699e-02, -4.1549149361e-02, 1.5420585824e-02, 5.4413218064e-02, -1.3991942256e-02, -7.3511311682e-02, 9.8306147091e-03, 1.0538098831e-01, 2.4767895457e-03, -1.7013983793e-01, -5.3519295197e-02, 3.5729875325e-01, 5.9530013734e-01, 3.5729875325e-01, -5.3519295197e-02, -1.7013983793e-01, 2.4767895457e-03, 1.0538098831e-01, 9.8306147091e-03, -7.3511311682e-02, -1.3991942256e-02, 5.4413218064e-02, 1.5420585824e-02, -4.1549149361e-02, -1.5638192699e-02, 3.2223155290e-02, 1.5213119554e-02, -2.5135038845e-02, -1.4406996028e-02, 1.9583979076e-02, 1.3362938210e-02, -1.5158439259e-02, -1.2171477770e-02, 1.1598799243e-02, 1.0896800128e-02, -8.7303353142e-03, -9.5881685667e-03, 6.4277320980e-03, 8.2841916600e-03, -4.5973156063e-03, -7.0175713933e-03, 3.1633610776e-03, 5.8140101675e-03, -2.0632159584e-03, -4.6937909542e-03, 1.2430372921e-03, 3.6720962046e-03, -6.5548048055e-04, -2.7593013177e-03, 2.5839388583e-04, 1.9613024094e-03, -1.4106495666e-05, -1.2799048805e-03, -1.1093829736e-04, 7.1327754530e-04, 1.4635687092e-04, -2.5647044402e-04, -1.1791553911e-04, -9.8021126546e-05, 4.7605082323e-05, 3.5965967563e-04, 4.6206061884e-05, -5.3942758146e-04, -1.4890118284e-04, 6.4888144112e-04, 2.4899722791e-04, -6.9999979578e-04, -3.3816268708e-04, 7.0454923950e-04, 4.1083915738e-04, -6.7369245331e-04, -4.6388070868e-04, 6.1766717590e-04, 4.9616951008e-04, -5.4554763871e-04, -5.0822117940e-04, 4.6508817914e-04, 5.0179308310e-04, -3.8264756455e-04, -4.7951215941e-04, 3.0318466104e-04, 4.4453252714e-04, -2.3031764274e-04, -4.0023458429e-04, 1.6643173499e-04, 3.4996062912e-04, -1.1285857228e-04, -2.9687173927e-04, 6.9969543240e-05, 2.4371243539e-04, -3.7440682291e-05, -1.9277411182e-04, 1.4403443484e-05, 1.4583183818e-04, 3.7650515294e-07, -1.0413520871e-04, -8.3394585095e-06, 6.8433254890e-05, 1.0994139826e-05, -3.9025740577e-05, -9.8065695691e-06, 1.5831781930e-05, 6.1157778312e-06, 1.5310917825e-06, -1.0766754409e-06, -1.3665196308e-05, -4.3728197481e-06, 2.1313177486e-05, 9.5223982251e-06, -2.5287767236e-05, -1.3887984508e-05, 2.6400053739e-05, 1.7177613167e-05, -2.5425425753e-05, -1.9275907764e-05, 2.3056727953e-05, 2.0203022903e-05, -1.9883422927e-05, -2.0078608383e-05, 1.6380162505e-05, 1.9086646276e-05, -1.2905490830e-05, -1.7443962288e-05, 9.7084647273e-06, 1.5373724961e-05, -6.9412383420e-06, -1.3084991486e-05, 4.6752496998e-06, 1.0758341788e-05, -2.9191987685e-06, -8.5375061091e-06, 1.6368965465e-06, 6.5260611238e-06, -7.6397724595e-07, -4.7889010594e-06, 2.2117927620e-07, 3.3549988889e-06, 7.2763377227e-08, -2.2249377290e-06, -1.9384365527e-07, 1.3774224210e-06, 2.0712149544e-07, -7.7661761489e-07, -1.6394613230e-07, 3.7888894052e-07, 1.0133745579e-07, -1.3853077565e-07, -4.3002365521e-08, 1.2186120186e-08, 1.4084131418e-09, 3.8109090962e-08, 1.9581108834e-08, -4.3206471974e-08}
  COEFFICIENT_WIDTH 24
  QUANTIZATION Quantize_Only
  BESTPRECISION true
  FILTER_TYPE Decimation
  DECIMATION_RATE 2
  NUMBER_CHANNELS 1
  NUMBER_PATHS 4
  SAMPLE_FREQUENCY 30.72
  CLOCK_FREQUENCY 122.88
  OUTPUT_ROUNDING_MODE Convergent_Rounding_to_Even
  OUTPUT_WIDTH 18
  M_DATA_HAS_TREADY true
  HAS_ARESETN true
} {
  S_AXIS_DATA comb_0/M_AXIS
  aclk pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter subset_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 12
  M_TDATA_NUM_BYTES 8
  TDATA_REMAP {tdata[87:72],tdata[63:48],tdata[39:24],tdata[15:0]}
} {
  S_AXIS fir_0/M_AXIS_DATA
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
  AXIS_TDATA_WIDTH 64
  FIFO_WRITE_DEPTH 512
} {
  S_AXIS subset_0/M_AXIS
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
    A dds_[expr $i + 2]/dout
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
