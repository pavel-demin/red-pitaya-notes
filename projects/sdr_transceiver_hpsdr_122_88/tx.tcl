# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 8 DIN_FROM 2 DIN_TO 2
}

cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 96 DIN_FROM 31 DIN_TO 0
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 96 DIN_FROM 47 DIN_TO 32
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 96 DIN_FROM 63 DIN_TO 48
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_4 {
  DIN_WIDTH 96 DIN_FROM 79 DIN_TO 64
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_0 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 64
  WRITE_DEPTH 4096
  ALWAYS_VALID TRUE
} {
  aclk /pll_0/clk_out1
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter subset_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 8
  M_TDATA_NUM_BYTES 8
  TDATA_REMAP {tdata[7:0],tdata[15:8],tdata[23:16],tdata[31:24],tdata[39:32],tdata[47:40],tdata[55:48],tdata[63:56]}
} {
  S_AXIS fifo_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_dwidth_converter
cell xilinx.com:ip:axis_dwidth_converter conv_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 8
  M_TDATA_NUM_BYTES 2
} {
  S_AXIS subset_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create fir_compiler
cell xilinx.com:ip:fir_compiler fir_0 {
  DATA_WIDTH.VALUE_SRC USER
  DATA_WIDTH 16
  COEFFICIENTVECTOR {-1.6476206008e-08, -4.7319580742e-08, -7.9381067905e-10, 3.0932292392e-08, 1.8626889750e-08, 3.2746669966e-08, -6.3004129353e-09, -1.5227093206e-07, -8.3038617413e-08, 3.1451709451e-07, 3.0560484117e-07, -4.7414525581e-07, -7.1344726202e-07, 5.4729313816e-07, 1.3345350134e-06, -4.1411962847e-07, -2.1503722734e-06, -6.7730495315e-08, 3.0752695159e-06, 1.0369498784e-06, -3.9441147447e-06, -2.5917416239e-06, 4.5150908731e-06, 4.7475177999e-06, -4.4925601114e-06, -7.3977704100e-06, 3.5719387440e-06, 1.0288872737e-05, -1.5037254527e-06, -1.3020034373e-05, -1.8319635139e-06, 1.5077192350e-05, 6.3542502968e-06, -1.5904670136e-05, -1.1731745321e-05, 1.5010039829e-05, 1.7370887571e-05, -1.2093550900e-05, -2.2465462507e-05, 7.1692396209e-06, 2.6101608873e-05, -6.6346893873e-07, -2.7427519537e-05, -6.5502877473e-06, 2.5862669520e-05, 1.3203568420e-05, -2.1315607549e-05, -1.7788980059e-05, 1.4365372007e-05, 1.8818539487e-05, -6.3571297138e-06, -1.5161566083e-05, -6.3471384237e-07, 6.4153591681e-06, 4.0077797995e-06, 6.7572099213e-06, -1.0055855105e-06, -2.2401706115e-05, -1.0761672174e-05, 3.7231200972e-05, 3.2698481722e-05, -4.6857798965e-05, -6.4648335436e-05, 4.6257373671e-05, 1.0439507646e-04, -3.0538825022e-05, -1.4744776012e-04, -4.1194858149e-06, 1.8717402344e-04, 5.9468128208e-05, -2.1536130974e-04, -1.3429319990e-04, 2.2321009173e-04, 2.2382079284e-04, -2.0268789753e-04, -3.1960318233e-04, 1.4808689370e-04, 4.1002412730e-04, -5.7556146579e-05, -4.8148420815e-04, -6.5672228671e-05, 5.2021767779e-04, 2.1265085309e-04, -5.1458560100e-04, -3.6898268416e-04, 4.5743765779e-04, 5.1594117619e-04, -3.4845480810e-04, -6.3284127546e-04, 1.9563599809e-04, 7.0014421907e-04, -1.5796522767e-05, -7.0316024196e-04, -1.6636819143e-04, 6.3580243619e-04, 3.2081412793e-04, -5.0377720838e-04, -4.1623109951e-04, 3.2654648069e-04, 4.2536677611e-04, -1.3745452555e-04, -3.3102561254e-04, -1.8435631049e-05, 1.3194484983e-04, 8.8990960308e-05, 1.5239764865e-04, -2.2010176557e-05, -4.7908123150e-04, -2.2613898157e-04, 7.8155034555e-04, 6.8028795312e-04, -9.7378507826e-04, -1.3373417278e-03, 9.5745121052e-04, 2.1580979991e-03, -6.3302572185e-04, -3.0621999990e-03, -8.6264164026e-05, 3.9272431628e-03, 1.2588134247e-03, -4.5929111950e-03, -2.8988737720e-03, 4.8705018499e-03, 4.9623956619e-03, -4.5575939788e-03, -7.3362016769e-03, 3.4569450875e-03, 9.8319072513e-03, -1.3981032650e-03, -1.2185384373e-02, -1.7402370575e-03, 1.4061380795e-02, 6.0081628428e-03, -1.5064603004e-02, -1.1369429891e-02, 1.4747394486e-02, 1.7686114637e-02, -1.2617681237e-02, -2.4711601090e-02, 8.1303202061e-03, 3.2084422372e-02, -6.4501187772e-04, -3.9314506127e-02, -1.0691866586e-02, 4.5733621697e-02, 2.7248622397e-02, -5.0318013968e-02, -5.1712770129e-02, 5.1015768284e-02, 9.0565292941e-02, -4.1604612534e-02, -1.6373694206e-01, -1.0801943921e-02, 3.5636037787e-01, 5.5477485410e-01, 3.5636037787e-01, -1.0801943921e-02, -1.6373694206e-01, -4.1604612534e-02, 9.0565292941e-02, 5.1015768284e-02, -5.1712770129e-02, -5.0318013968e-02, 2.7248622397e-02, 4.5733621697e-02, -1.0691866586e-02, -3.9314506127e-02, -6.4501187772e-04, 3.2084422372e-02, 8.1303202061e-03, -2.4711601090e-02, -1.2617681237e-02, 1.7686114637e-02, 1.4747394486e-02, -1.1369429891e-02, -1.5064603004e-02, 6.0081628428e-03, 1.4061380795e-02, -1.7402370575e-03, -1.2185384373e-02, -1.3981032650e-03, 9.8319072513e-03, 3.4569450875e-03, -7.3362016769e-03, -4.5575939788e-03, 4.9623956619e-03, 4.8705018499e-03, -2.8988737720e-03, -4.5929111950e-03, 1.2588134247e-03, 3.9272431628e-03, -8.6264164026e-05, -3.0621999990e-03, -6.3302572185e-04, 2.1580979991e-03, 9.5745121052e-04, -1.3373417278e-03, -9.7378507826e-04, 6.8028795312e-04, 7.8155034555e-04, -2.2613898157e-04, -4.7908123150e-04, -2.2010176557e-05, 1.5239764865e-04, 8.8990960308e-05, 1.3194484983e-04, -1.8435631049e-05, -3.3102561254e-04, -1.3745452555e-04, 4.2536677611e-04, 3.2654648069e-04, -4.1623109951e-04, -5.0377720838e-04, 3.2081412793e-04, 6.3580243619e-04, -1.6636819143e-04, -7.0316024196e-04, -1.5796522767e-05, 7.0014421907e-04, 1.9563599809e-04, -6.3284127546e-04, -3.4845480810e-04, 5.1594117619e-04, 4.5743765779e-04, -3.6898268416e-04, -5.1458560100e-04, 2.1265085309e-04, 5.2021767779e-04, -6.5672228671e-05, -4.8148420815e-04, -5.7556146579e-05, 4.1002412730e-04, 1.4808689370e-04, -3.1960318233e-04, -2.0268789753e-04, 2.2382079284e-04, 2.2321009173e-04, -1.3429319990e-04, -2.1536130974e-04, 5.9468128208e-05, 1.8717402344e-04, -4.1194858149e-06, -1.4744776012e-04, -3.0538825022e-05, 1.0439507646e-04, 4.6257373671e-05, -6.4648335436e-05, -4.6857798965e-05, 3.2698481722e-05, 3.7231200972e-05, -1.0761672174e-05, -2.2401706115e-05, -1.0055855105e-06, 6.7572099213e-06, 4.0077797995e-06, 6.4153591680e-06, -6.3471384237e-07, -1.5161566083e-05, -6.3571297138e-06, 1.8818539487e-05, 1.4365372007e-05, -1.7788980059e-05, -2.1315607549e-05, 1.3203568420e-05, 2.5862669520e-05, -6.5502877473e-06, -2.7427519537e-05, -6.6346893873e-07, 2.6101608873e-05, 7.1692396209e-06, -2.2465462507e-05, -1.2093550900e-05, 1.7370887571e-05, 1.5010039829e-05, -1.1731745321e-05, -1.5904670136e-05, 6.3542502968e-06, 1.5077192350e-05, -1.8319635139e-06, -1.3020034373e-05, -1.5037254527e-06, 1.0288872737e-05, 3.5719387440e-06, -7.3977704100e-06, -4.4925601114e-06, 4.7475177999e-06, 4.5150908731e-06, -2.5917416239e-06, -3.9441147447e-06, 1.0369498784e-06, 3.0752695159e-06, -6.7730495315e-08, -2.1503722734e-06, -4.1411962847e-07, 1.3345350134e-06, 5.4729313816e-07, -7.1344726202e-07, -4.7414525581e-07, 3.0560484117e-07, 3.1451709451e-07, -8.3038617413e-08, -1.5227093206e-07, -6.3004129353e-09, 3.2746669966e-08, 1.8626889750e-08, 3.0932292392e-08, -7.9381067905e-10, -4.7319580742e-08, -1.6476206008e-08}
  COEFFICIENT_WIDTH 24
  QUANTIZATION Quantize_Only
  BESTPRECISION true
  FILTER_TYPE Interpolation
  INTERPOLATION_RATE 2
  NUMBER_CHANNELS 4
  NUMBER_PATHS 1
  SAMPLE_FREQUENCY 0.048
  CLOCK_FREQUENCY 122.88
  OUTPUT_ROUNDING_MODE Convergent_Rounding_to_Even
  OUTPUT_WIDTH 25
  M_DATA_HAS_TREADY true
  HAS_ARESETN true
} {
  S_AXIS_DATA conv_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter subset_1 {
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

# Create axis_dwidth_converter
cell xilinx.com:ip:axis_dwidth_converter conv_1 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 3
  M_TDATA_NUM_BYTES 12
} {
  S_AXIS subset_1/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster bcast_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 12
  M_TDATA_NUM_BYTES 6
  M00_TDATA_REMAP {tdata[47:0]}
  M01_TDATA_REMAP {tdata[95:48]}
} {
  S_AXIS conv_1/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  USE_BYTE_WRITE_ENABLE true
  BYTE_SIZE 8
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 1024
  WRITE_WIDTH_B 32
  REGISTER_PORTA_OUTPUT_OF_MEMORY_PRIMITIVES false
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
}

# Create axis_keyer
cell pavel-demin:user:axis_keyer keyer_0 {
  AXIS_TDATA_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 10
} {
  B_BRAM bram_0/BRAM_PORTB
  cfg_data slice_2/dout
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter subset_2 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 6
  TDATA_REMAP {24'b000000000000000000000000,tdata[23:0]}
} {
  S_AXIS keyer_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_selector
cell pavel-demin:user:axis_selector sel_0 {
  AXIS_TDATA_WIDTH 48
} {
  S00_AXIS bcast_0/M00_AXIS
  S01_AXIS subset_2/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster bcast_1 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 6
  M_TDATA_NUM_BYTES 3
  M00_TDATA_REMAP {tdata[23:0]}
  M01_TDATA_REMAP {tdata[47:24]}
} {
  S_AXIS sel_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster bcast_2 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 6
  M_TDATA_NUM_BYTES 3
  M00_TDATA_REMAP {tdata[23:0]}
  M01_TDATA_REMAP {tdata[47:24]}
} {
  S_AXIS bcast_0/M01_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

for {set i 0} {$i <= 3} {incr i} {

  # Create cic_compiler
  cell xilinx.com:ip:cic_compiler cic_$i {
    INPUT_DATA_WIDTH.VALUE_SRC USER
    FILTER_TYPE Interpolation
    NUMBER_OF_STAGES 6
    FIXED_OR_INITIAL_RATE 1280
    INPUT_SAMPLE_FREQUENCY 0.096
    CLOCK_FREQUENCY 122.88
    INPUT_DATA_WIDTH 24
    QUANTIZATION Truncation
    OUTPUT_DATA_WIDTH 24
    USE_XTREME_DSP_SLICE false
    HAS_ARESETN true
  } {
    S_AXIS_DATA bcast_[expr $i / 2 + 1]/M0[expr $i % 2]_AXIS
    aclk /pll_0/clk_out1
    aresetn /rst_0/peripheral_aresetn
  }

}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 3
} {
  S00_AXIS cic_0/M_AXIS_DATA
  S01_AXIS cic_1/M_AXIS_DATA
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_constant
cell pavel-demin:user:axis_constant phase_0 {
  AXIS_TDATA_WIDTH 32
} {
  cfg_data slice_1/dout
  aclk /pll_0/clk_out1
}

# Create dds_compiler
cell xilinx.com:ip:dds_compiler dds_0 {
  DDS_CLOCK_RATE 122.88
  SPURIOUS_FREE_DYNAMIC_RANGE 138
  FREQUENCY_RESOLUTION 0.2
  PHASE_INCREMENT Streaming
  HAS_ARESETN true
  HAS_PHASE_OUT false
  PHASE_WIDTH 30
  OUTPUT_WIDTH 24
  DSP48_USE Minimal
} {
  S_AXIS_PHASE phase_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_lfsr
cell pavel-demin:user:axis_lfsr lfsr_0 {} {
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create cmpy
cell xilinx.com:ip:cmpy mult_0 {
  APORTWIDTH.VALUE_SRC USER
  BPORTWIDTH.VALUE_SRC USER
  APORTWIDTH 24
  BPORTWIDTH 24
  ROUNDMODE Random_Rounding
  OUTPUTWIDTH 26
} {
  S_AXIS_A comb_0/M_AXIS
  s_axis_b_tdata dds_0/m_axis_data_tdata
  s_axis_b_tvalid dds_0/m_axis_data_tvalid
  S_AXIS_CTRL lfsr_0/M_AXIS
  aclk /pll_0/clk_out1
}

# Create dsp48
cell pavel-demin:user:dsp48 mult_1 {
  A_WIDTH 24
  B_WIDTH 16
  P_WIDTH 16
} {
  A mult_0/m_axis_dout_tdata
  B slice_3/dout
  CLK /pll_0/clk_out1
}

# Create dsp48
cell pavel-demin:user:dsp48 mult_2 {
  A_WIDTH 24
  B_WIDTH 16
  P_WIDTH 18
} {
  A mult_0/m_axis_dout_tdata
  B slice_4/dout
  CLK /pll_0/clk_out1
}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner comb_1 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 3
} {
  S00_AXIS cic_2/M_AXIS_DATA
  S01_AXIS cic_3/M_AXIS_DATA
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create cordic
cell xilinx.com:ip:cordic cordic_0 {
  INPUT_WIDTH.VALUE_SRC USER
  FUNCTIONAL_SELECTION Translate
  PIPELINING_MODE Optimal
  INPUT_WIDTH 24
  OUTPUT_WIDTH 15
  ROUND_MODE Round_Pos_Neg_Inf
  COMPENSATION_SCALING BRAM
} {
  S_AXIS_CARTESIAN comb_1/M_AXIS
  aclk /pll_0/clk_out1
}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  NUM_PORTS 2
  IN0_WIDTH 16
  IN1_WIDTH 16
} {
  In0 mult_1/P
  In1 cordic_0/m_axis_dout_tdata
}

for {set i 0} {$i <= 1} {incr i} {

  # Create port_slicer
  cell pavel-demin:user:port_slicer slice_[expr $i + 5] {
    DIN_WIDTH 96 DIN_FROM [expr $i + 80] DIN_TO [expr $i + 80]
  }

  # Create port_selector
  cell pavel-demin:user:port_selector selector_$i {
    DOUT_WIDTH 16
  } {
    cfg slice_[expr $i + 5]/dout
    din concat_0/dout
  }

}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_1 {
  NUM_PORTS 2
  IN0_WIDTH 16
  IN1_WIDTH 16
} {
  In0 selector_0/dout
  In1 selector_1/dout
}
