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

# Create clk_wiz
cell xilinx.com:ip:clk_wiz:5.2 pll_0 {
  PRIMITIVE PLL
  PRIM_IN_FREQ.VALUE_SRC USER
  PRIM_IN_FREQ 125.0
  CLKOUT1_USED true
  CLKOUT2_USED true
  CLKOUT1_REQUESTED_OUT_FREQ 125.0
  CLKOUT2_REQUESTED_OUT_FREQ 250.0
} {
  clk_in1 adc_0/adc_clk
}

# Create axis_red_pitaya_dac
cell pavel-demin:user:axis_red_pitaya_dac:1.0 dac_0 {} {
  aclk pll_0/clk_out1
  ddr_clk pll_0/clk_out2
  locked pll_0/locked
  dac_clk dac_clk_o
  dac_rst dac_rst_o
  dac_sel dac_sel_o
  dac_wrt dac_wrt_o
  dac_dat dac_dat_o
}

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register:1.0 cfg_0 {
  CFG_DATA_WIDTH 448
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_0 {
  DIN_WIDTH 448 DIN_FROM 7 DIN_TO 0 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_1 {
  DIN_WIDTH 448 DIN_FROM 15 DIN_TO 8 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_2 {
  DIN_WIDTH 448 DIN_FROM 23 DIN_TO 16 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_3 {
  DIN_WIDTH 448 DIN_FROM 31 DIN_TO 24 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# rate_0/cfg_data and rate_1/cfg_data

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_1 {
  DIN_WIDTH 448 DIN_FROM 47 DIN_TO 32 DOUT_WIDTH 16
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_0 {
  DIN_WIDTH 448 DIN_FROM 191 DIN_TO 64 DOUT_WIDTH 128
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_1 {
  DIN_WIDTH 448 DIN_FROM 319 DIN_TO 192 DOUT_WIDTH 128
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_2 {
  DIN_WIDTH 448 DIN_FROM 415 DIN_TO 320 DOUT_WIDTH 96
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_3 {
  DIN_WIDTH 448 DIN_FROM 447 DIN_TO 416 DOUT_WIDTH 32
} {
  Din cfg_0/cfg_data
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0

# Create axis_clock_converter
cell xilinx.com:ip:axis_clock_converter:1.1 fifo_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
} {
  S_AXIS adc_0/M_AXIS
  s_axis_aclk adc_0/adc_clk
  s_axis_aresetn const_0/dout
  m_axis_aclk ps_0/FCLK_CLK0
  m_axis_aresetn rst_0/peripheral_aresetn
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster:1.1 bcast_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 2
  NUM_MI 4
  M00_TDATA_REMAP {tdata[15:0]}
  M01_TDATA_REMAP {tdata[31:16]}
  M02_TDATA_REMAP {16'b0000000000000000}
  M03_TDATA_REMAP {16'b0000000000000000}
} {
  S_AXIS fifo_0/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_variable
cell pavel-demin:user:axis_variable:1.0 rate_0 {
  AXIS_TDATA_WIDTH 16
} {
  cfg_data slice_1/Dout
  aclk /ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_variable
cell pavel-demin:user:axis_variable:1.0 rate_1 {
  AXIS_TDATA_WIDTH 16
} {
  cfg_data slice_1/Dout
  aclk /ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create cic_compiler
cell xilinx.com:ip:cic_compiler:4.0 cic_0 {
  INPUT_DATA_WIDTH.VALUE_SRC USER
  FILTER_TYPE Decimation
  NUMBER_OF_STAGES 6
  SAMPLE_RATE_CHANGES Programmable
  MINIMUM_RATE 5
  MAXIMUM_RATE 8192
  FIXED_OR_INITIAL_RATE 5
  INPUT_SAMPLE_FREQUENCY 125
  CLOCK_FREQUENCY 125
  INPUT_DATA_WIDTH 14
  QUANTIZATION Truncation
  OUTPUT_DATA_WIDTH 16
  USE_XTREME_DSP_SLICE false
  HAS_DOUT_TREADY true
  HAS_ARESETN true
} {
  S_AXIS_DATA bcast_0/M00_AXIS
  S_AXIS_CONFIG rate_0/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create cic_compiler
cell xilinx.com:ip:cic_compiler:4.0 cic_1 {
  INPUT_DATA_WIDTH.VALUE_SRC USER
  FILTER_TYPE Decimation
  NUMBER_OF_STAGES 6
  SAMPLE_RATE_CHANGES Programmable
  MINIMUM_RATE 5
  MAXIMUM_RATE 8192
  FIXED_OR_INITIAL_RATE 5
  INPUT_SAMPLE_FREQUENCY 125
  CLOCK_FREQUENCY 125
  INPUT_DATA_WIDTH 14
  QUANTIZATION Truncation
  OUTPUT_DATA_WIDTH 16
  USE_XTREME_DSP_SLICE false
  HAS_DOUT_TREADY true
  HAS_ARESETN true
} {
  S_AXIS_DATA bcast_0/M01_AXIS
  S_AXIS_CONFIG rate_1/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner:1.1 comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 2
} {
  S00_AXIS cic_0/M_AXIS_DATA
  S01_AXIS cic_1/M_AXIS_DATA
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create fir_compiler
cell xilinx.com:ip:fir_compiler:7.2 fir_0 {
  DATA_WIDTH.VALUE_SRC USER
  DATA_WIDTH 16
  COEFFICIENTVECTOR {-1.63744122654648e-08, -4.62216716148361e-08, -3.14335779482632e-10, 3.02310153778336e-08, 1.74890850381969e-08, 3.19482286529326e-08, -5.0994421994774e-09, -1.48653353613353e-07, -8.26761935440861e-08, 3.07015823012366e-07, 3.0080886332324e-07, -4.62653063559883e-07, -7.00184893242507e-07, 5.33506302697611e-07, 1.30813357034074e-06, -4.023225322563e-07, -2.10652650584874e-06, -7.01725431628003e-08, 3.01150223729751e-06, 1.01967031117686e-06, -3.86150372394275e-06, -2.54225818072438e-06, 4.41993348941294e-06, 4.65304341627651e-06, -4.39753212538659e-06, -7.24787717681163e-06, 3.49626558808964e-06, 1.0078701336886e-05, -1.47194685779028e-06, -1.27534942409821e-05, -1.79286233594637e-06, 1.47694078057067e-05, 6.21926306560833e-06, -1.5582846847983e-05, -1.14833577256531e-05, 1.47120230922135e-05, 1.70048621091381e-05, -1.18634714639314e-05, -2.19954283147527e-05, 7.05059419354727e-06, 2.55614060867368e-05, -6.89990820946048e-07, -2.68697800541403e-05, -6.36451714718008e-06, 2.53527750074515e-05, 1.28728450795703e-05, -2.0920846677794e-05, -1.73612517441299e-05, 1.41401071263444e-05, 1.83749862506759e-05, -6.32585991158657e-06, -1.48087387385988e-05, -4.93474951076127e-07, 6.26916256752369e-06, 3.77320372706693e-06, 6.59496436596694e-06, -8.18014683171042e-07, -2.18717457405279e-05, -1.07087232130912e-05, 3.63461490752955e-05, 3.21800806140094e-05, -4.57260582225158e-05, -6.34419626433276e-05, 4.5097399118797e-05, 0.000102325890655008, -2.96761791743105e-05, -0.00014443842428366, -4.27564702687111e-06, 0.000183291485733018, 5.84704971571537e-05, -0.000210851185795382, -0.000131721288143241, 0.000218509180683351, 0.000219358868548782, -0.000198406916678391, -0.000313120548592492, 0.000144959004714433, 0.000401644120432757, -5.63518653243416e-05, -0.000471626935842207, -6.42569578034905e-05, 0.000509602506151267, 0.000208120065222889, -0.000504183059896151, -0.000361158959377472, 0.000448371031054935, 0.000505061696434652, -0.000341847567026698, -0.0006195990988342, 0.000192428194445537, 0.000685661560487131, -1.65505528881502e-05, -0.000688880371015645, -0.000161641019845141, 0.00062329894459272, 0.000312770999488855, -0.000494493181298271, -0.000406224936898567, 0.000321490786537126, 0.000415356137374431, -0.000136895740494839, -0.000323352565488648, -1.52271175908499e-05, 0.000128983871136271, 8.38978661849253e-05, 0.000148695344532513, -1.8069608877988e-05, -0.000467701726518259, -0.000224921620006527, 0.000762940586986797, 0.000669359852735534, -0.000950271910714701, -0.00131221385225305, 0.000933507406901155, 0.00211513707866678, -0.000615289331550511, -0.00299952994722302, -8.92798158239705e-05, 0.00384566229126372, 0.00123735556393418, -0.00449671320994515, -0.00284293452198785, 0.00476808031019541, 0.00486300922613273, -0.0044617146685581, -0.00718698064468655, 0.00338458990770112, 0.00963071343738828, -0.0013698211464178, -0.0119360152269839, -0.00170150377088473, 0.0137751539824885, 0.00587879694622239, -0.0147616583115625, -0.0111272087562733, 0.014457882763948, 0.0173128033439249, -0.012382722980173, -0.0241959325972279, 0.00800309669893809, 0.031425705539797, -0.000692615375256258, -0.0385276197048089, -0.010384517623774, 0.0448576874366831, 0.0265700861070664, -0.0494359449279653, -0.050506107204478, 0.0503104495147679, 0.0885922374355363, -0.0415657042866342, -0.160704610785777, -0.00838308000357802, 0.354565700905743, 0.550459213055103, 0.354565700905743, -0.00838308000357801, -0.160704610785777, -0.0415657042866342, 0.0885922374355363, 0.0503104495147679, -0.0505061072044779, -0.0494359449279653, 0.0265700861070664, 0.0448576874366831, -0.0103845176237741, -0.0385276197048088, -0.000692615375256252, 0.031425705539797, 0.00800309669893806, -0.0241959325972279, -0.0123827229801729, 0.0173128033439249, 0.014457882763948, -0.0111272087562733, -0.0147616583115624, 0.00587879694622238, 0.0137751539824885, -0.00170150377088473, -0.0119360152269839, -0.00136982114641781, 0.00963071343738828, 0.00338458990770112, -0.00718698064468655, -0.00446171466855811, 0.00486300922613271, 0.00476808031019541, -0.00284293452198785, -0.00449671320994515, 0.00123735556393418, 0.00384566229126372, -8.92798158239772e-05, -0.00299952994722302, -0.000615289331550518, 0.00211513707866678, 0.000933507406901159, -0.00131221385225306, -0.000950271910714696, 0.000669359852735537, 0.000762940586986796, -0.00022492162000653, -0.000467701726518248, -1.80696088779863e-05, 0.0001486953445325, 8.38978661849229e-05, 0.000128983871136272, -1.5227117590845e-05, -0.000323352565488649, -0.000136895740494844, 0.000415356137374436, 0.00032149078653713, -0.000406224936898562, -0.000494493181298275, 0.000312770999488843, 0.000623298944592722, -0.000161641019845136, -0.000688880371015644, -1.6550552888178e-05, 0.000685661560487131, 0.000192428194445553, -0.0006195990988342, -0.000341847567026709, 0.000505061696434649, 0.000448371031054938, -0.000361158959377469, -0.000504183059896154, 0.000208120065222887, 0.000509602506151268, -6.42569578034892e-05, -0.000471626935842202, -5.63518653243425e-05, 0.000401644120432759, 0.000144959004714433, -0.00031312054859249, -0.000198406916678392, 0.000219358868548782, 0.00021850918068335, -0.000131721288143243, -0.000210851185795382, 5.84704971571526e-05, 0.000183291485733017, -4.27564702687076e-06, -0.000144438424283659, -2.96761791743111e-05, 0.000102325890655008, 4.50973991187948e-05, -6.34419626433272e-05, -4.57260582225145e-05, 3.21800806140092e-05, 3.63461490752936e-05, -1.07087232130911e-05, -2.18717457405289e-05, -8.18014683171156e-07, 6.59496436596684e-06, 3.77320372706711e-06, 6.26916256752231e-06, -4.93474951076179e-07, -1.48087387385992e-05, -6.32585991158642e-06, 1.83749862506765e-05, 1.41401071263443e-05, -1.73612517441294e-05, -2.09208466777939e-05, 1.28728450795703e-05, 2.53527750074514e-05, -6.36451714717938e-06, -2.68697800541402e-05, -6.8999082094604e-07, 2.55614060867367e-05, 7.05059419354764e-06, -2.19954283147527e-05, -1.18634714639311e-05, 1.70048621091382e-05, 1.47120230922136e-05, -1.14833577256531e-05, -1.55828468479833e-05, 6.21926306560826e-06, 1.47694078057067e-05, -1.79286233594635e-06, -1.27534942409822e-05, -1.47194685779027e-06, 1.00787013368863e-05, 3.49626558808964e-06, -7.2478771768118e-06, -4.39753212538663e-06, 4.65304341627645e-06, 4.41993348941295e-06, -2.54225818072432e-06, -3.86150372394272e-06, 1.01967031117682e-06, 3.01150223729752e-06, -7.01725431628033e-08, -2.10652650584875e-06, -4.02322532256309e-07, 1.30813357034074e-06, 5.33506302697589e-07, -7.00184893242498e-07, -4.62653063559866e-07, 3.0080886332324e-07, 3.07015823012349e-07, -8.26761935440843e-08, -1.48653353613346e-07, -5.09944219947347e-09, 3.19482286529264e-08, 1.74890850381962e-08, 3.02310153778422e-08, -3.14335779482123e-10, -4.62216716148339e-08, -1.63744122654657e-08}
  COEFFICIENT_WIDTH 16
  QUANTIZATION Maximize_Dynamic_Range
  BESTPRECISION true
  FILTER_TYPE Decimation
  DECIMATION_RATE 2
  NUMBER_PATHS 2
  SAMPLE_FREQUENCY 25
  CLOCK_FREQUENCY 125
  OUTPUT_ROUNDING_MODE Non_Symmetric_Rounding_Up
  OUTPUT_WIDTH 16
  M_DATA_HAS_TREADY true
  HAS_ARESETN true
} {
  S_AXIS_DATA comb_0/M_AXIS
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster:1.1 bcast_1 {
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
  aclk ps_0/FCLK_CLK0
  aresetn rst_0/peripheral_aresetn
}

module hst_0 {
  source projects/mcpha/hst.tcl
} {
  slice_0/Din rst_slice_0/Dout
  slice_1/Din rst_slice_0/Dout
  slice_2/Din rst_slice_0/Dout
  slice_3/Din rst_slice_0/Dout
  slice_4/Din cfg_slice_0/Dout
  slice_5/Din cfg_slice_0/Dout
  slice_6/Din cfg_slice_0/Dout
  slice_7/Din cfg_slice_0/Dout
  timer_0/S_AXIS bcast_0/M02_AXIS
  pha_0/S_AXIS bcast_1/M00_AXIS
}

module hst_1 {
  source projects/mcpha/hst.tcl
} {
  slice_0/Din rst_slice_1/Dout
  slice_1/Din rst_slice_1/Dout
  slice_2/Din rst_slice_1/Dout
  slice_3/Din rst_slice_1/Dout
  slice_4/Din cfg_slice_1/Dout
  slice_5/Din cfg_slice_1/Dout
  slice_6/Din cfg_slice_1/Dout
  slice_7/Din cfg_slice_1/Dout
  timer_0/S_AXIS bcast_0/M03_AXIS
  pha_0/S_AXIS bcast_1/M01_AXIS
}

module osc_0 {
  source projects/mcpha/osc.tcl
} {
  slice_0/Din rst_slice_2/Dout
  slice_1/Din rst_slice_2/Dout
  slice_2/Din rst_slice_2/Dout
  slice_3/Din rst_slice_2/Dout
  slice_4/Din cfg_slice_2/Dout
  slice_5/Din cfg_slice_2/Dout
  slice_6/Din cfg_slice_2/Dout
  switch_0/S00_AXIS bcast_1/M02_AXIS
  switch_0/S01_AXIS bcast_1/M03_AXIS
  comb_0/S00_AXIS bcast_1/M04_AXIS
  comb_0/S01_AXIS bcast_1/M05_AXIS
}

module gen_0 {
  source projects/mcpha/gen.tcl
} {
  slice_0/Din rst_slice_3/Dout
  slice_1/Din cfg_slice_3/Dout
  fifo_1/M_AXIS dac_0/S_AXIS
}

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_0 {
  NUM_PORTS 4
  IN0_WIDTH 64
  IN1_WIDTH 64
  IN2_WIDTH 32
  IN3_WIDTH 32
} {
  In0 hst_0/timer_0/sts_data
  In1 hst_1/timer_0/sts_data
  In2 osc_0/scope_0/sts_data
  In3 osc_0/writer_0/sts_data
}

# Create axi_sts_register
cell pavel-demin:user:axi_sts_register:1.0 sts_0 {
  STS_DATA_WIDTH 192
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
} {
  sts_data concat_0/dout
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins cfg_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_cfg_0_reg0]
set_property OFFSET 0x40000000 [get_bd_addr_segs ps_0/Data/SEG_cfg_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins sts_0/S_AXI]

set_property RANGE 4K [get_bd_addr_segs ps_0/Data/SEG_sts_0_reg0]
set_property OFFSET 0x40001000 [get_bd_addr_segs ps_0/Data/SEG_sts_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
}  [get_bd_intf_pins osc_0/switch_0/S_AXI_CTRL]

set_property RANGE 4K [get_bd_addr_segs {ps_0/Data/SEG_switch_0_Reg}]
set_property OFFSET 0x40002000 [get_bd_addr_segs {ps_0/Data/SEG_switch_0_Reg}]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins hst_0/reader_0/S_AXI]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]
set_property OFFSET 0x40010000 [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg0]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins hst_1/reader_0/S_AXI]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg01]
set_property OFFSET 0x40020000 [get_bd_addr_segs ps_0/Data/SEG_reader_0_reg01]

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
  Master /ps_0/M_AXI_GP0
  Clk Auto
} [get_bd_intf_pins gen_0/writer_0/S_AXI]

set_property RANGE 64K [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]
set_property OFFSET 0x40030000 [get_bd_addr_segs ps_0/Data/SEG_writer_0_reg0]
