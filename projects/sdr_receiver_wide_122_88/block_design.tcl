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

  # Create axis_constant
  cell pavel-demin:user:axis_constant phase_$i {
    AXIS_TDATA_WIDTH 32
  } {
    cfg_data slice_[expr $i + 4]/dout
    aclk pll_0/clk_out1
  }

  # Create dds_compiler
  cell xilinx.com:ip:dds_compiler dds_$i {
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
    MINIMUM_RATE 6
    MAXIMUM_RATE 64
    FIXED_OR_INITIAL_RATE 6
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
  COEFFICIENTVECTOR {-1.6404667985e-08, -4.6551698245e-08, -4.5902585194e-10, 3.0441809577e-08, 1.7831969627e-08, 3.2188245129e-08, -5.4617617159e-09, -1.4974081176e-07, -8.2783151967e-08, 3.0927081158e-07, 3.0224754335e-07, -4.6610804164e-07, -7.0416717880e-07, 5.3765179692e-07, 1.3160636650e-06, -4.0587137076e-07, -2.1196982635e-06, -6.9433344204e-08, 3.0306602369e-06, 1.0248556136e-06, -3.8863243336e-06, -2.5571186138e-06, 4.4485246296e-06, 4.6814211929e-06, -4.4260850602e-06, -7.2929055560e-06, 3.5190034652e-06, 1.0141840164e-05, -1.4814959735e-06, -1.2833568280e-05, -1.8046104012e-06, 1.4861871677e-05, 6.2598218318e-06, -1.5679524695e-05, -1.1557989106e-05, 1.4801541766e-05, 1.7114837773e-05, -1.1932569327e-05, -2.2136650648e-05, 7.0862003425e-06, 2.5723703468e-05, -6.8196881271e-07, -2.7037333433e-05, -6.4203975665e-06, 2.5505933644e-05, 1.2972285706e-05, -2.1039386641e-05, -1.7489840758e-05, 1.4207689036e-05, 1.8508324147e-05, -6.3351189097e-06, -1.4914800723e-05, -5.3608402555e-07, 6.3131117252e-06, 3.8438901395e-06, 6.6437307970e-06, -8.7460394630e-07, -2.2031048175e-05, -1.0724386277e-05, 3.6612202407e-05, 3.2335593514e-05, -4.6066295845e-05, -6.3804200655e-05, 4.5446181735e-05, 1.0294740290e-04, -2.9935680140e-05, -1.4534246075e-04, -4.2283759227e-06, 1.8445793899e-04, 5.8769870260e-05, -2.1220625857e-04, -1.3249366170e-04, 2.1992162440e-04, 2.2069911973e-04, -1.9969321974e-04, -3.1506795094e-04, 1.4589886084e-04, 4.0416160898e-04, -5.6713744225e-05, -4.7458826363e-04, -6.4682175144e-05, 5.1279149644e-04, 2.0948139789e-04, -5.0730806282e-04, -3.6350970101e-04, 4.5109449031e-04, 5.0833054045e-04, -3.4383187609e-04, -6.2357773984e-04, 1.9339087067e-04, 6.9001272162e-04, -1.6322610036e-05, -6.9317028968e-04, -1.6306295019e-04, 6.2705469028e-04, 3.1518938366e-04, -4.9728103912e-04, -4.0923314829e-04, 3.2300752813e-04, 4.1836553013e-04, -1.3706060703e-04, -3.2565922320e-04, -1.6194910163e-05, 1.2987411523e-04, 8.5432566128e-05, 1.4980801965e-04, -1.9258542978e-05, -4.7112219365e-04, -2.2528215303e-04, 7.6853471616e-04, 6.7263807590e-04, -9.5734067069e-04, -1.3197588735e-03, 9.4070690011e-04, 2.1280408876e-03, -6.2062497436e-04, -3.0183565333e-03, -8.8366151475e-05, 3.8701720135e-03, 1.2437944840e-03, -4.5256160793e-03, -2.8597333081e-03, 4.7988543433e-03, 4.8928620190e-03, -4.4905240369e-03, -7.2318067623e-03, 3.4063318511e-03, 9.6911552567e-03, -1.3783210349e-03, -1.2010931376e-02, -1.7131397719e-03, 1.3861143020e-02, 5.9176652657e-03, -1.4852667553e-02, -1.1199986626e-02, 1.4544850646e-02, 1.7424969235e-02, -1.2453290739e-02, -2.4350870317e-02, 8.0412798224e-03, 3.1623619164e-02, -6.7824624933e-04, -3.8764030819e-02, -1.0476969331e-02, 4.5120823142e-02, 2.6774134647e-02, -4.9700844781e-02, -5.0868973380e-02, 5.0522005012e-02, 8.9185670367e-02, -4.1576062049e-02, -1.6161675329e-01, -9.1175847665e-03, 3.5508931028e-01, 5.5173671490e-01, 3.5508931028e-01, -9.1175847665e-03, -1.6161675329e-01, -4.1576062049e-02, 8.9185670367e-02, 5.0522005012e-02, -5.0868973380e-02, -4.9700844781e-02, 2.6774134647e-02, 4.5120823142e-02, -1.0476969331e-02, -3.8764030819e-02, -6.7824624933e-04, 3.1623619164e-02, 8.0412798224e-03, -2.4350870317e-02, -1.2453290739e-02, 1.7424969235e-02, 1.4544850646e-02, -1.1199986626e-02, -1.4852667553e-02, 5.9176652657e-03, 1.3861143020e-02, -1.7131397719e-03, -1.2010931376e-02, -1.3783210349e-03, 9.6911552567e-03, 3.4063318511e-03, -7.2318067623e-03, -4.4905240369e-03, 4.8928620190e-03, 4.7988543433e-03, -2.8597333081e-03, -4.5256160793e-03, 1.2437944840e-03, 3.8701720135e-03, -8.8366151475e-05, -3.0183565333e-03, -6.2062497436e-04, 2.1280408876e-03, 9.4070690011e-04, -1.3197588735e-03, -9.5734067069e-04, 6.7263807590e-04, 7.6853471616e-04, -2.2528215303e-04, -4.7112219365e-04, -1.9258542978e-05, 1.4980801965e-04, 8.5432566128e-05, 1.2987411523e-04, -1.6194910163e-05, -3.2565922320e-04, -1.3706060703e-04, 4.1836553013e-04, 3.2300752813e-04, -4.0923314829e-04, -4.9728103912e-04, 3.1518938366e-04, 6.2705469028e-04, -1.6306295019e-04, -6.9317028968e-04, -1.6322610036e-05, 6.9001272162e-04, 1.9339087067e-04, -6.2357773984e-04, -3.4383187609e-04, 5.0833054045e-04, 4.5109449031e-04, -3.6350970101e-04, -5.0730806282e-04, 2.0948139789e-04, 5.1279149644e-04, -6.4682175144e-05, -4.7458826363e-04, -5.6713744225e-05, 4.0416160898e-04, 1.4589886084e-04, -3.1506795094e-04, -1.9969321974e-04, 2.2069911973e-04, 2.1992162440e-04, -1.3249366170e-04, -2.1220625857e-04, 5.8769870260e-05, 1.8445793899e-04, -4.2283759227e-06, -1.4534246075e-04, -2.9935680140e-05, 1.0294740290e-04, 4.5446181735e-05, -6.3804200655e-05, -4.6066295845e-05, 3.2335593514e-05, 3.6612202407e-05, -1.0724386277e-05, -2.2031048175e-05, -8.7460394630e-07, 6.6437307970e-06, 3.8438901395e-06, 6.3131117252e-06, -5.3608402555e-07, -1.4914800723e-05, -6.3351189097e-06, 1.8508324147e-05, 1.4207689036e-05, -1.7489840758e-05, -2.1039386641e-05, 1.2972285706e-05, 2.5505933644e-05, -6.4203975665e-06, -2.7037333433e-05, -6.8196881271e-07, 2.5723703468e-05, 7.0862003425e-06, -2.2136650648e-05, -1.1932569327e-05, 1.7114837773e-05, 1.4801541766e-05, -1.1557989106e-05, -1.5679524695e-05, 6.2598218318e-06, 1.4861871677e-05, -1.8046104012e-06, -1.2833568280e-05, -1.4814959735e-06, 1.0141840164e-05, 3.5190034652e-06, -7.2929055560e-06, -4.4260850602e-06, 4.6814211929e-06, 4.4485246296e-06, -2.5571186138e-06, -3.8863243336e-06, 1.0248556136e-06, 3.0306602369e-06, -6.9433344204e-08, -2.1196982635e-06, -4.0587137076e-07, 1.3160636650e-06, 5.3765179692e-07, -7.0416717880e-07, -4.6610804164e-07, 3.0224754335e-07, 3.0927081158e-07, -8.2783151967e-08, -1.4974081176e-07, -5.4617617159e-09, 3.2188245129e-08, 1.7831969627e-08, 3.0441809577e-08, -4.5902585194e-10, -4.6551698245e-08, -1.6404667985e-08}
  COEFFICIENT_WIDTH 24
  QUANTIZATION Quantize_Only
  BESTPRECISION true
  FILTER_TYPE Decimation
  DECIMATION_RATE 2
  NUMBER_CHANNELS 1
  NUMBER_PATHS 4
  SAMPLE_FREQUENCY 20.48
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
