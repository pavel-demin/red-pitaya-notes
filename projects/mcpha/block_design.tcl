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
}

# Create axi_hub
cell pavel-demin:user:axi_hub hub_0 {
  CFG_DATA_WIDTH 800
  STS_DATA_WIDTH 352
} {
  S_AXI ps_0/M_AXI_GP0
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_0 {
  DIN_WIDTH 800 DIN_FROM 7 DIN_TO 0
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer sel_slice_0 {
  DIN_WIDTH 800 DIN_FROM 3 DIN_TO 3
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer neg_slice_0 {
  DIN_WIDTH 800 DIN_FROM 4 DIN_TO 4
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_1 {
  DIN_WIDTH 800 DIN_FROM 15 DIN_TO 8
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer neg_slice_1 {
  DIN_WIDTH 800 DIN_FROM 12 DIN_TO 12
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_2 {
  DIN_WIDTH 800 DIN_FROM 23 DIN_TO 16
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_3 {
  DIN_WIDTH 800 DIN_FROM 31 DIN_TO 24
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer sel_slice_1 {
  DIN_WIDTH 800 DIN_FROM 27 DIN_TO 27
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer neg_slice_2 {
  DIN_WIDTH 800 DIN_FROM 28 DIN_TO 28
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer neg_slice_3 {
  DIN_WIDTH 800 DIN_FROM 29 DIN_TO 29
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_4 {
  DIN_WIDTH 800 DIN_FROM 30 DIN_TO 30
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_5 {
  DIN_WIDTH 800 DIN_FROM 31 DIN_TO 31
} {
  din hub_0/cfg_data
}

# rate_0/cfg_data and rate_1/cfg_data

# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 800 DIN_FROM 47 DIN_TO 32
} {
  din hub_0/cfg_data
}

# rate_2/cfg_data and rate_3/cfg_data

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 800 DIN_FROM 63 DIN_TO 48
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_0 {
  DIN_WIDTH 800 DIN_FROM 191 DIN_TO 64
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_1 {
  DIN_WIDTH 800 DIN_FROM 319 DIN_TO 192
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_2 {
  DIN_WIDTH 800 DIN_FROM 447 DIN_TO 320
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_3 {
  DIN_WIDTH 800 DIN_FROM 575 DIN_TO 448
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_4 {
  DIN_WIDTH 800 DIN_FROM 703 DIN_TO 576
} {
  din hub_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_5 {
  DIN_WIDTH 800 DIN_FROM 799 DIN_TO 704
} {
  din hub_0/cfg_data
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster bcast_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 2
  NUM_MI 12
  M00_TDATA_REMAP {tdata[15:0]}
  M01_TDATA_REMAP {tdata[15:0]}
  M02_TDATA_REMAP {tdata[31:16]}
  M03_TDATA_REMAP {tdata[31:16]}
  M04_TDATA_REMAP {tdata[15:0]}
  M05_TDATA_REMAP {tdata[15:0]}
  M06_TDATA_REMAP {tdata[31:16]}
  M07_TDATA_REMAP {tdata[31:16]}
  M08_TDATA_REMAP {16'b0000000000000000}
  M09_TDATA_REMAP {16'b0000000000000000}
  M10_TDATA_REMAP {16'b0000000000000000}
  M11_TDATA_REMAP {16'b0000000000000000}
} {
  S_AXIS adc_0/M_AXIS
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

for {set i 0} {$i <= 7} {incr i} {

  # Create axis_negator
  cell pavel-demin:user:axis_negator neg_${i} {
    AXIS_TDATA_WIDTH 16
  } {
    S_AXIS bcast_0/M0${i}_AXIS
    cfg_flag neg_slice_[expr $i / 2]/dout
    aclk pll_0/clk_out1
  }

}

for {set i 0} {$i <= 3} {incr i} {

  # Create axis_variable
  cell pavel-demin:user:axis_variable rate_${i} {
    AXIS_TDATA_WIDTH 16
  } {
    cfg_data slice_[expr $i / 2]/dout
    aclk pll_0/clk_out1
    aresetn rst_0/peripheral_aresetn
  }

  # Create cic_compiler
  cell xilinx.com:ip:cic_compiler cic_${i} {
    INPUT_DATA_WIDTH.VALUE_SRC USER
    FILTER_TYPE Decimation
    NUMBER_OF_STAGES 6
    SAMPLE_RATE_CHANGES Programmable
    MINIMUM_RATE 4
    MAXIMUM_RATE 8192
    FIXED_OR_INITIAL_RATE 4
    INPUT_SAMPLE_FREQUENCY 125
    CLOCK_FREQUENCY 125
    INPUT_DATA_WIDTH 14
    QUANTIZATION Truncation
    OUTPUT_DATA_WIDTH 14
    USE_XTREME_DSP_SLICE false
    HAS_ARESETN true
  } {
    S_AXIS_DATA neg_[expr 2 * $i + 1]/M_AXIS
    S_AXIS_CONFIG rate_${i}/M_AXIS
    aclk pll_0/clk_out1
    aresetn rst_0/peripheral_aresetn
  }

  # Create axis_selector
  cell pavel-demin:user:axis_selector sel_${i} {
    AXIS_TDATA_WIDTH 16
  } {
    S00_AXIS neg_[expr 2 * $i + 0]/M_AXIS
    S01_AXIS cic_${i}/M_AXIS_DATA
    cfg_data sel_slice_[expr $i / 2]/dout
    aclk pll_0/clk_out1
    aresetn rst_0/peripheral_aresetn
  }

}

for {set i 0} {$i <= 1} {incr i} {

  # Create axis_combiner
  cell  xilinx.com:ip:axis_combiner comb_${i} {
    TDATA_NUM_BYTES.VALUE_SRC USER
    TDATA_NUM_BYTES 2
  } {
    S00_AXIS sel_[expr 2 * $i + 0]/M_AXIS
    S01_AXIS sel_[expr 2 * $i + 1]/M_AXIS
    aclk pll_0/clk_out1
    aresetn rst_0/peripheral_aresetn
  }

  # Create fir_compiler
  cell xilinx.com:ip:fir_compiler fir_${i} {
    DATA_WIDTH.VALUE_SRC USER
    DATA_WIDTH 14
    COEFFICIENTVECTOR {5.2264535203e-06, 5.7185303335e-05, 2.6647653637e-04, 7.9231931667e-04, 1.8933556984e-03, 3.9077032410e-03, 7.2107962593e-03, 1.2147384825e-02, 1.8943280784e-02, 2.7612738319e-02, 3.7884778843e-02, 4.9172940009e-02, 6.0606317648e-02, 7.1126384126e-02, 7.9637295055e-02, 8.5182038247e-02, 8.7107558671e-02, 8.5182038247e-02, 7.9637295055e-02, 7.1126384126e-02, 6.0606317648e-02, 4.9172940009e-02, 3.7884778843e-02, 2.7612738319e-02, 1.8943280784e-02, 1.2147384825e-02, 7.2107962593e-03, 3.9077032410e-03, 1.8933556984e-03, 7.9231931667e-04, 2.6647653637e-04, 5.7185303335e-05, 5.2264535203e-06}
    COEFFICIENT_WIDTH 16
    QUANTIZATION Quantize_Only
    BESTPRECISION true
    NUMBER_PATHS 2
    SAMPLE_FREQUENCY 125
    CLOCK_FREQUENCY 125
    OUTPUT_ROUNDING_MODE Non_Symmetric_Rounding_Up
    OUTPUT_WIDTH 14
    HAS_ARESETN true
  } {
    S_AXIS_DATA comb_${i}/M_AXIS
    aclk pll_0/clk_out1
    aresetn rst_0/peripheral_aresetn
  }

}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster bcast_1 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 2
  NUM_MI 6
  M00_TDATA_REMAP {tdata[15:0]}
  M01_TDATA_REMAP {tdata[31:16]}
  M02_TDATA_REMAP {tdata[15:0]}
  M03_TDATA_REMAP {tdata[31:16]}
  M04_TDATA_REMAP {tdata[15:0]}
  M05_TDATA_REMAP {tdata[31:16]}
} {
  S_AXIS fir_0/M_AXIS_DATA
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster bcast_2 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 2
  M00_TDATA_REMAP {tdata[15:0]}
  M01_TDATA_REMAP {tdata[31:16]}
} {
  S_AXIS fir_1/M_AXIS_DATA
  aclk pll_0/clk_out1
  aresetn rst_0/peripheral_aresetn
}

module pha_0 {
  source projects/mcpha/pha.tcl
} {
  slice_0/din rst_slice_0/dout
  slice_1/din rst_slice_0/dout
  slice_2/din rst_slice_0/dout
  slice_3/din cfg_slice_0/dout
  slice_4/din cfg_slice_0/dout
  slice_5/din cfg_slice_0/dout
  slice_6/din cfg_slice_0/dout
  timer_0/S_AXIS bcast_0/M08_AXIS
  pha_0/S_AXIS bcast_1/M00_AXIS
}

module hst_0 {
  source projects/mcpha/hst.tcl
} {
  slice_0/din rst_slice_0/dout
  hst_0/S_AXIS pha_0/vldtr_0/M_AXIS
}

module pha_1 {
  source projects/mcpha/pha.tcl
} {
  slice_0/din rst_slice_1/dout
  slice_1/din rst_slice_1/dout
  slice_2/din rst_slice_1/dout
  slice_3/din cfg_slice_1/dout
  slice_4/din cfg_slice_1/dout
  slice_5/din cfg_slice_1/dout
  slice_6/din cfg_slice_1/dout
  timer_0/S_AXIS bcast_0/M09_AXIS
  pha_0/S_AXIS bcast_1/M01_AXIS
}

module hst_1 {
  source projects/mcpha/hst.tcl
} {
  slice_0/din rst_slice_1/dout
  hst_0/S_AXIS pha_1/vldtr_0/M_AXIS
}

module osc_0 {
  source projects/mcpha/osc.tcl
} {
  slice_0/din rst_slice_2/dout
  slice_1/din rst_slice_2/dout
  slice_2/din rst_slice_2/dout
  slice_3/din rst_slice_2/dout
  slice_4/din rst_slice_2/dout
  slice_5/din rst_slice_2/dout
  slice_6/din cfg_slice_4/dout
  slice_7/din cfg_slice_4/dout
  slice_8/din cfg_slice_4/dout
  slice_9/din cfg_slice_4/dout
  sel_0/S00_AXIS bcast_1/M02_AXIS
  sel_0/S01_AXIS bcast_1/M03_AXIS
  comb_0/S00_AXIS bcast_1/M04_AXIS
  comb_0/S01_AXIS bcast_1/M05_AXIS
  writer_0/M_AXI ps_0/S_AXI_ACP
}

module pha_2 {
  source projects/mcpha/pha.tcl
} {
  slice_0/din rst_slice_3/dout
  slice_1/din rst_slice_3/dout
  slice_2/din rst_slice_3/dout
  slice_3/din cfg_slice_2/dout
  slice_4/din cfg_slice_2/dout
  slice_5/din cfg_slice_2/dout
  slice_6/din cfg_slice_2/dout
  timer_0/S_AXIS bcast_0/M10_AXIS
  pha_0/S_AXIS bcast_2/M00_AXIS
  vldtr_0/m_axis_tready const_0/dout
}

module pha_3 {
  source projects/mcpha/pha.tcl
} {
  slice_0/din rst_slice_3/dout
  slice_1/din rst_slice_3/dout
  slice_2/din rst_slice_3/dout
  slice_3/din cfg_slice_3/dout
  slice_4/din cfg_slice_3/dout
  slice_5/din cfg_slice_3/dout
  slice_6/din cfg_slice_3/dout
  timer_0/S_AXIS bcast_0/M11_AXIS
  pha_0/S_AXIS bcast_2/M01_AXIS
  vldtr_0/m_axis_tready const_0/dout
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 64 DIN_FROM 31 DIN_TO 0
} {
  din pha_2/timer_0/sts_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 64 DIN_FROM 63 DIN_TO 32
} {
  din pha_2/timer_0/sts_data
}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  NUM_PORTS 4
  IN0_WIDTH 32
  IN1_WIDTH 32
  IN2_WIDTH 32
  IN3_WIDTH 32
} {
  In0 pha_3/vldtr_0/m_axis_tdata
  In1 pha_2/vldtr_0/m_axis_tdata
  In2 slice_3/dout
  In3 slice_2/dout
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic or_0 {
  C_SIZE 1
  C_OPERATION or
} {
  Op1 pha_2/vldtr_0/m_axis_tvalid
  Op2 pha_3/vldtr_0/m_axis_tvalid
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_0 {
  S_AXIS_TDATA_WIDTH 128
  M_AXIS_TDATA_WIDTH 32
  WRITE_DEPTH 2048
} {
  s_axis_tdata concat_0/dout
  s_axis_tvalid or_0/Res
  M_AXIS hub_0/S00_AXIS
  aclk pll_0/clk_out1
  aresetn rst_slice_4/dout
}

module gen_0 {
  source projects/mcpha/gen.tcl
} {
  slice_0/din rst_slice_5/dout
  slice_1/din cfg_slice_5/dout
  iir_0/M_AXIS dac_0/S_AXIS
}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_1 {
  NUM_PORTS 8
  IN0_WIDTH 64
  IN1_WIDTH 64
  IN2_WIDTH 64
  IN3_WIDTH 64
  IN4_WIDTH 32
  IN5_WIDTH 32
  IN6_WIDTH 16
  IN7_WIDTH 16
} {
  In0 pha_0/timer_0/sts_data
  In1 pha_1/timer_0/sts_data
  In2 pha_2/timer_0/sts_data
  In3 pha_3/timer_0/sts_data
  In4 osc_0/scope_0/sts_data
  In5 osc_0/writer_0/sts_data
  In6 fifo_0/read_count
  In7 gen_0/fifo_0/write_count
  dout hub_0/sts_data
}

wire gen_0/fifo_0/S_AXIS hub_0/M00_AXIS
wire hst_0/bram_0/BRAM_PORTA hub_0/B01_BRAM
wire hst_1/bram_0/BRAM_PORTA hub_0/B02_BRAM
