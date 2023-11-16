create_bd_port -dir I clk
create_bd_port -dir I -from 31 -to 0 cfg_data
create_bd_port -dir O -from 31 -to 0 sts_data

# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 32 DIN_FROM 15 DIN_TO 0
} {
  din cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 32 DIN_FROM 31 DIN_TO 16
} {
  din cfg_data
}

# Create dsp_macro
cell xilinx.com:ip:dsp_macro dsp_0 {
  INSTRUCTION1 A+C
  A_WIDTH.VALUE_SRC USER
  C_WIDTH.VALUE_SRC USER
  OUTPUT_PROPERTIES User_Defined
  A_WIDTH 16
  C_WIDTH 16
  P_WIDTH 32
} {
  A slice_0/dout
  C slice_1/dout
  P sts_data
  CLK clk
}
