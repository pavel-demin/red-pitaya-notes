# Create clk_wiz
cell xilinx.com:ip:clk_wiz pll_0 {
  PRIMITIVE PLL
  PRIM_IN_FREQ.VALUE_SRC USER
  PRIM_IN_FREQ 125.0
  PRIM_SOURCE Differential_clock_capable_pin
  CLKOUT1_USED true
  CLKOUT1_REQUESTED_OUT_FREQ 125.0
  USE_RESET false
} {
  clk_in1_p adc_clk_p_i
  clk_in1_n adc_clk_n_i
}

# Create processing_system7
cell xilinx.com:ip:processing_system7 ps_0 {
  PCW_IMPORT_BOARD_PRESET cfg/red_pitaya.xml
} {
  M_AXI_GP0_ACLK pll_0/clk_out1
}

# Create all required interconnections
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {
  make_external {FIXED_IO, DDR}
  Master Disable
  Slave Disable
} [get_bd_cells ps_0]

# Create xlconstant
cell xilinx.com:ip:xlconstant const_0

# SATA connectors

delete_bd_objs [get_bd_ports adc_enc_p_o]
delete_bd_objs [get_bd_ports adc_enc_n_o]

create_bd_port -dir I daisy_trg_p_i
create_bd_port -dir I daisy_trg_n_i

create_bd_port -dir I daisy_clk_p_i
create_bd_port -dir I daisy_clk_n_i

create_bd_port -dir O daisy_trg_p_o
create_bd_port -dir O daisy_trg_n_o

create_bd_port -dir O daisy_clk_p_o
create_bd_port -dir O daisy_clk_n_o

create_bd_port -dir O adc_clk_p_o
create_bd_port -dir O adc_clk_n_o

# Create util_ds_buf
cell xilinx.com:ip:util_ds_buf trg_buf_0 {
  C_SIZE 1
  C_BUF_TYPE OBUFDS
} {
  OBUF_DS_P daisy_trg_p_o
  OBUF_DS_N daisy_trg_n_o
  OBUF_IN const_0/dout
}

# Create util_ds_buf
cell xilinx.com:ip:util_ds_buf trg_buf_1 {
  C_SIZE 1
  C_BUF_TYPE IBUFDS
} {
  IBUF_DS_P daisy_trg_p_i
  IBUF_DS_N daisy_trg_n_i
}

# Create util_ds_buf
cell xilinx.com:ip:util_ds_buf clk_buf_0 {
  C_SIZE 1
  C_BUF_TYPE IBUFDS
} {
  IBUF_DS_P daisy_clk_p_i
  IBUF_DS_N daisy_clk_n_i
}

# Create util_ds_buf
cell xilinx.com:ip:util_ds_buf clk_buf_1 {
  C_SIZE 1
  C_BUF_TYPE OBUFDS
} {
  OBUF_DS_P daisy_clk_p_o
  OBUF_DS_N daisy_clk_n_o
  OBUF_IN clk_buf_0/IBUF_OUT
}

# Create util_ds_buf
cell xilinx.com:ip:util_ds_buf clk_buf_2 {
  C_SIZE 1
  C_BUF_TYPE OBUFDS
} {
  OBUF_DS_P adc_clk_p_o
  OBUF_DS_N adc_clk_n_o
  OBUF_IN clk_buf_0/IBUF_OUT
}

# LED

# Create c_counter_binary
cell xilinx.com:ip:c_counter_binary cntr_0 {
  Output_Width 32
} {
  CLK pll_0/clk_out1
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 32 DIN_FROM 26 DIN_TO 26
} {
  din cntr_0/Q
  dout led_o
}
