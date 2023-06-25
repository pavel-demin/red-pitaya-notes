# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 128 DIN_FROM 15 DIN_TO 0
}

for {set i 0} {$i <= 3} {incr i} {

  # Create port_slicer
  cell pavel-demin:user:port_slicer slice_[expr $i + 4] {
    DIN_WIDTH 128 DIN_FROM [expr $i + 16] DIN_TO [expr $i + 16]
  }

  # Create port_selector
  cell pavel-demin:user:port_selector selector_$i {
    DOUT_WIDTH 16
  } {
    cfg slice_[expr $i + 4]/dout
    din /adc_0/m_axis_tdata
  }

}

for {set i 0} {$i <= 1} {incr i} {

  # Create port_slicer
  cell pavel-demin:user:port_slicer slice_[expr $i + 1] {
    DIN_WIDTH 8 DIN_FROM $i DIN_TO $i
  }

  # Create port_slicer
  cell pavel-demin:user:port_slicer slice_[expr $i + 8] {
    DIN_WIDTH 128 DIN_FROM [expr 32 * $i + 63] DIN_TO [expr 32 * $i + 32]
  }

  # Create axis_constant
  cell pavel-demin:user:axis_constant phase_$i {
    AXIS_TDATA_WIDTH 32
  } {
    cfg_data slice_[expr $i + 8]/dout
    aclk /pll_0/clk_out1
  }

  # Create dds_compiler
  cell xilinx.com:ip:dds_compiler dds_$i {
    DDS_CLOCK_RATE 122.88
    SPURIOUS_FREE_DYNAMIC_RANGE 138
    FREQUENCY_RESOLUTION 0.2
    PHASE_INCREMENT Streaming
    HAS_ARESETN true
    HAS_PHASE_OUT false
    PHASE_WIDTH 30
    OUTPUT_WIDTH 24
    DSP48_USE Minimal
    NEGATIVE_SINE true
  } {
    S_AXIS_PHASE phase_$i/M_AXIS
    aclk /pll_0/clk_out1
    aresetn slice_[expr $i + 1]/dout
  }

}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_10 {
  DIN_WIDTH 128 DIN_FROM 127 DIN_TO 96
}

# Create axis_constant
cell pavel-demin:user:axis_constant phase_2 {
  AXIS_TDATA_WIDTH 32
} {
  cfg_data slice_10/dout
  aclk /pll_0/clk_out1
}

# Create dds_compiler
cell xilinx.com:ip:dds_compiler dds_2 {
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
  S_AXIS_PHASE phase_2/M_AXIS
  aclk /pll_0/clk_out1
}

for {set i 0} {$i <= 5} {incr i} {

  # Create port_slicer
  cell pavel-demin:user:port_slicer dds_slice_$i {
    DIN_WIDTH 48 DIN_FROM [expr 24 * ($i % 2) + 23] DIN_TO [expr 24 * ($i % 2)]
  } {
    din dds_[expr $i / 2]/m_axis_data_tdata
  }

}

for {set i 0} {$i <= 3} {incr i} {

  # Create port_slicer
  cell pavel-demin:user:port_slicer dds_slice_[expr $i + 6] {
    DIN_WIDTH 48 DIN_FROM [expr 47 - 24 * ($i % 2)] DIN_TO [expr 24 - 24 * ($i % 2)]
  }

}

for {set i 0} {$i <= 7} {incr i} {

  # Create port_slicer
  cell pavel-demin:user:port_slicer adc_slice_$i {
    DIN_WIDTH 16 DIN_FROM 15 DIN_TO 0
  } {
    din selector_[expr $i / 2]/dout
  }

}

for {set i 0} {$i <= 1} {incr i} {

  # Create port_slicer
  cell pavel-demin:user:port_slicer adc_slice_[expr $i + 8] {
    DIN_WIDTH 16 DIN_FROM 15 DIN_TO 0
  }

}

# Create xlconstant
cell xilinx.com:ip:xlconstant const_0

for {set i 0} {$i <= 9} {incr i} {

  # Create dsp48
  cell pavel-demin:user:dsp48 mult_$i {
    A_WIDTH 24
    B_WIDTH 16
    P_WIDTH 24
  } {
    A dds_slice_$i/dout
    B adc_slice_$i/dout
    CLK /pll_0/clk_out1
  }

  # Create axis_variable
  cell pavel-demin:user:axis_variable rate_$i {
    AXIS_TDATA_WIDTH 16
  } {
    cfg_data slice_3/dout
    aclk /pll_0/clk_out1
    aresetn /rst_0/peripheral_aresetn
  }

  # Create cic_compiler
  cell xilinx.com:ip:cic_compiler cic_$i {
    INPUT_DATA_WIDTH.VALUE_SRC USER
    FILTER_TYPE Decimation
    NUMBER_OF_STAGES 6
    SAMPLE_RATE_CHANGES Programmable
    MINIMUM_RATE 160
    MAXIMUM_RATE 1280
    FIXED_OR_INITIAL_RATE 1280
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
    aresetn /rst_0/peripheral_aresetn
  }

}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
  NUM_SI 10
} {
  S00_AXIS cic_0/M_AXIS_DATA
  S01_AXIS cic_1/M_AXIS_DATA
  S02_AXIS cic_2/M_AXIS_DATA
  S03_AXIS cic_3/M_AXIS_DATA
  S04_AXIS cic_4/M_AXIS_DATA
  S05_AXIS cic_5/M_AXIS_DATA
  S06_AXIS cic_6/M_AXIS_DATA
  S07_AXIS cic_7/M_AXIS_DATA
  S08_AXIS cic_8/M_AXIS_DATA
  S09_AXIS cic_9/M_AXIS_DATA
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_dwidth_converter
cell xilinx.com:ip:axis_dwidth_converter conv_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 40
  M_TDATA_NUM_BYTES 4
} {
  S_AXIS comb_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create fir_compiler
cell xilinx.com:ip:fir_compiler fir_0 {
  DATA_WIDTH.VALUE_SRC USER
  DATA_WIDTH 32
  COEFFICIENTVECTOR {-1.6476107645e-08, -4.7318509958e-08, -7.9334153761e-10, 3.0931608556e-08, 1.8625777766e-08, 3.2745890947e-08, -6.2992383002e-09, -1.5226740325e-07, -8.3038268718e-08, 3.1450977739e-07, 3.0560017042e-07, -4.7413404537e-07, -7.1343433668e-07, 5.4727968794e-07, 1.3345092769e-06, -4.1410811573e-07, -2.1503295275e-06, -6.7732889107e-08, 3.0752073447e-06, 1.0369330452e-06, -3.9440341990e-06, -2.5916933930e-06, 4.5149980933e-06, 4.7474257033e-06, -4.4924674575e-06, -7.3976242805e-06, 3.5718649624e-06, 1.0288667838e-05, -1.5036944713e-06, -1.3019774519e-05, -1.8319253834e-06, 1.5076892291e-05, 6.3541186734e-06, -1.5904356404e-05, -1.1731503131e-05, 1.5009749328e-05, 1.7370530688e-05, -1.2093326664e-05, -2.2465004229e-05, 7.1691240631e-06, 2.6101082206e-05, -6.6349494949e-07, -2.7426975813e-05, -6.5501064436e-06, 2.5862172503e-05, 1.3203245774e-05, -2.1315222860e-05, -1.7788562838e-05, 1.4365152661e-05, 1.8818106866e-05, -6.3570996029e-06, -1.5161221980e-05, -6.3457567365e-07, 6.4152166211e-06, 4.0075505603e-06, 6.7570516180e-06, -1.0054020629e-06, -2.2401189151e-05, -1.0761621117e-05, 3.7230337647e-05, 3.2697976810e-05, -4.6856694979e-05, -6.4647159673e-05, 4.6256242042e-05, 1.0439305935e-04, -3.0537983209e-05, -1.4744482623e-04, -4.1196388391e-06, 1.8717023806e-04, 5.9467156273e-05, -2.1535691238e-04, -1.3429069300e-04, 2.2320550831e-04, 2.2381644313e-04, -2.0268372358e-04, -3.1959686240e-04, 1.4808384411e-04, 4.1001595749e-04, -5.7554972650e-05, -4.8147459816e-04, -6.5670848380e-05, 5.2020732914e-04, 2.1264643506e-04, -5.1457546013e-04, -3.6897505552e-04, 4.5742882004e-04, 5.1593056840e-04, -3.4844836896e-04, -6.3282836452e-04, 1.9563287413e-04, 7.0013009948e-04, -1.5797262215e-05, -7.0314632126e-04, -1.6636357778e-04, 6.3579024895e-04, 3.2080628120e-04, -5.0376816185e-04, -4.1622133931e-04, 3.2654155861e-04, 4.2535701264e-04, -1.3745398968e-04, -3.3101812992e-04, -1.8432492399e-05, 1.3194196375e-04, 8.8985983325e-05, 1.5239403533e-04, -2.2006323174e-05, -4.7907013006e-04, -2.2613780527e-04, 7.8153219188e-04, 6.8027730730e-04, -9.7376214155e-04, -1.3373172349e-03, 9.5742785253e-04, 2.1580561165e-03, -6.3300841569e-04, -3.0621388977e-03, -8.6267116792e-05, 3.9271636219e-03, 1.2587925143e-03, -4.5928174022e-03, -2.8988192404e-03, 4.8704019911e-03, 4.9622987682e-03, -4.5575005029e-03, -7.3360561955e-03, 3.4568745542e-03, 9.8317111012e-03, -1.3980757112e-03, -1.2185141263e-02, -1.7401992634e-03, 1.4061101768e-02, 6.0080366772e-03, -1.5064307706e-02, -1.1369193695e-02, 1.4747112328e-02, 1.7685750649e-02, -1.2617452322e-02, -2.4711098353e-02, 8.1301963965e-03, 3.2083780259e-02, -6.4505858596e-04, -3.9313739227e-02, -1.0691566598e-02, 4.5732768283e-02, 2.7247960597e-02, -5.0317155092e-02, -5.1711593898e-02, 5.1015082518e-02, 9.0563371328e-02, -4.1604576660e-02, -1.6373399431e-01, -1.0799599424e-02, 3.5635861240e-01, 5.5477063056e-01, 3.5635861240e-01, -1.0799599424e-02, -1.6373399431e-01, -4.1604576660e-02, 9.0563371328e-02, 5.1015082518e-02, -5.1711593898e-02, -5.0317155092e-02, 2.7247960597e-02, 4.5732768283e-02, -1.0691566598e-02, -3.9313739227e-02, -6.4505858596e-04, 3.2083780259e-02, 8.1301963965e-03, -2.4711098353e-02, -1.2617452322e-02, 1.7685750649e-02, 1.4747112328e-02, -1.1369193695e-02, -1.5064307706e-02, 6.0080366772e-03, 1.4061101768e-02, -1.7401992634e-03, -1.2185141263e-02, -1.3980757112e-03, 9.8317111012e-03, 3.4568745542e-03, -7.3360561955e-03, -4.5575005029e-03, 4.9622987682e-03, 4.8704019911e-03, -2.8988192404e-03, -4.5928174022e-03, 1.2587925143e-03, 3.9271636219e-03, -8.6267116792e-05, -3.0621388977e-03, -6.3300841569e-04, 2.1580561165e-03, 9.5742785253e-04, -1.3373172349e-03, -9.7376214155e-04, 6.8027730730e-04, 7.8153219188e-04, -2.2613780527e-04, -4.7907013006e-04, -2.2006323174e-05, 1.5239403533e-04, 8.8985983325e-05, 1.3194196375e-04, -1.8432492399e-05, -3.3101812992e-04, -1.3745398968e-04, 4.2535701264e-04, 3.2654155861e-04, -4.1622133931e-04, -5.0376816185e-04, 3.2080628120e-04, 6.3579024895e-04, -1.6636357778e-04, -7.0314632126e-04, -1.5797262215e-05, 7.0013009948e-04, 1.9563287413e-04, -6.3282836452e-04, -3.4844836896e-04, 5.1593056840e-04, 4.5742882004e-04, -3.6897505552e-04, -5.1457546013e-04, 2.1264643506e-04, 5.2020732914e-04, -6.5670848380e-05, -4.8147459816e-04, -5.7554972650e-05, 4.1001595749e-04, 1.4808384411e-04, -3.1959686240e-04, -2.0268372358e-04, 2.2381644313e-04, 2.2320550831e-04, -1.3429069300e-04, -2.1535691238e-04, 5.9467156273e-05, 1.8717023806e-04, -4.1196388391e-06, -1.4744482623e-04, -3.0537983209e-05, 1.0439305935e-04, 4.6256242042e-05, -6.4647159673e-05, -4.6856694979e-05, 3.2697976810e-05, 3.7230337647e-05, -1.0761621117e-05, -2.2401189151e-05, -1.0054020629e-06, 6.7570516180e-06, 4.0075505603e-06, 6.4152166211e-06, -6.3457567365e-07, -1.5161221980e-05, -6.3570996029e-06, 1.8818106866e-05, 1.4365152661e-05, -1.7788562838e-05, -2.1315222860e-05, 1.3203245774e-05, 2.5862172503e-05, -6.5501064436e-06, -2.7426975813e-05, -6.6349494949e-07, 2.6101082206e-05, 7.1691240631e-06, -2.2465004229e-05, -1.2093326664e-05, 1.7370530688e-05, 1.5009749328e-05, -1.1731503131e-05, -1.5904356404e-05, 6.3541186734e-06, 1.5076892291e-05, -1.8319253834e-06, -1.3019774519e-05, -1.5036944713e-06, 1.0288667838e-05, 3.5718649624e-06, -7.3976242805e-06, -4.4924674575e-06, 4.7474257033e-06, 4.5149980933e-06, -2.5916933930e-06, -3.9440341990e-06, 1.0369330452e-06, 3.0752073447e-06, -6.7732889107e-08, -2.1503295275e-06, -4.1410811573e-07, 1.3345092769e-06, 5.4727968794e-07, -7.1343433668e-07, -4.7413404537e-07, 3.0560017042e-07, 3.1450977739e-07, -8.3038268718e-08, -1.5226740325e-07, -6.2992383002e-09, 3.2745890947e-08, 1.8625777766e-08, 3.0931608556e-08, -7.9334153761e-10, -4.7318509957e-08, -1.6476107645e-08}
  COEFFICIENT_WIDTH 24
  QUANTIZATION Quantize_Only
  BESTPRECISION true
  FILTER_TYPE Decimation
  DECIMATION_RATE 2
  NUMBER_CHANNELS 10
  NUMBER_PATHS 1
  SAMPLE_FREQUENCY 0.768
  CLOCK_FREQUENCY 122.88
  OUTPUT_ROUNDING_MODE Convergent_Rounding_to_Even
  OUTPUT_WIDTH 26
  HAS_ARESETN true
} {
  S_AXIS_DATA conv_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_dwidth_converter
cell xilinx.com:ip:axis_dwidth_converter conv_1 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 40
} {
  S_AXIS fir_0/M_AXIS_DATA
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter subset_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 40
  M_TDATA_NUM_BYTES 32
  TDATA_REMAP {16'b0000000000000000,tdata[263:256],tdata[271:264],tdata[279:272],tdata[295:288],tdata[303:296],tdata[311:304],tdata[199:192],tdata[207:200],tdata[215:208],tdata[231:224],tdata[239:232],tdata[247:240],tdata[135:128],tdata[143:136],tdata[151:144],tdata[167:160],tdata[175:168],tdata[183:176],tdata[71:64],tdata[79:72],tdata[87:80],tdata[103:96],tdata[111:104],tdata[119:112],tdata[7:0],tdata[15:8],tdata[23:16],tdata[39:32],tdata[47:40],tdata[55:48]}
} {
  S_AXIS conv_1/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_0 {
  S_AXIS_TDATA_WIDTH 256
  M_AXIS_TDATA_WIDTH 32
  WRITE_DEPTH 1024
  ALWAYS_READY TRUE
} {
  S_AXIS subset_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}
