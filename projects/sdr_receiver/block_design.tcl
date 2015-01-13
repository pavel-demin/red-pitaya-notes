source projects/led_blinker/block_design.tcl

cell xilinx.com:ip:dds_compiler:6.0 dds_0 {
  DDS_Clock_Rate 125
  Spurious_Free_Dynamic_Range 84
  Frequency_Resolution 10
  Phase_Increment Programmable
  Output_Selection Sine
  Memory_Type Block_ROM
  Optimization_Goal Speed
  DSP48_Use Maximal
  Has_Phase_Out false
  Phase_Width 24
  Output_Width 14
  Latency 10
  Output_Frequency1 0
  PINC1 0
} {
  S_AXIS_CONFIG cfg_0/M00_AXIS
  aclk ps_0/FCLK_CLK0
}

cell xilinx.com:ip:cmpy:6.0 cmpy_0 {
  APortWidth 14
  BPortWidth 14
  OptimizeGoal Performance
  RoundMode Truncate
  OutputWidth 28
  MinimumLatency 4
} {
  S_AXIS_A fifo_0/M_AXIS
  S_AXIS_B dds_0/M_AXIS_DATA
  aclk ps_0/FCLK_CLK0
}

cell xilinx.com:ip:cic_compiler:4.0 cic_0 {
  Filter_Type Decimation
  Fixed_Or_Initial_Rate 2048
  Input_Sample_Frequency 125
  Clock_Frequency 125
  Input_Data_Width 28
  Quantization Truncation
  Output_Data_Width 32
  Minimum_Rate 2048
  Maximum_Rate 2048
  SamplePeriod 1
} {
  S_AXIS_DATA cmpy_0/M_AXIS_DOUT
  aclk ps_0/FCLK_CLK0
}

