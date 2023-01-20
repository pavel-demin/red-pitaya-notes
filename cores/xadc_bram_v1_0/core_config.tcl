set display_name {XADC BRAM}

set core [ipx::current_core]

set_property DISPLAY_NAME $display_name $core
set_property DESCRIPTION $display_name $core

foreach port {Vp_Vn Vaux0 Vaux1 Vaux8 Vaux9} {
  set bus [ipx::add_bus_interface $port $core]
  set_property ABSTRACTION_TYPE_VLNV xilinx.com:interface:diff_analog_io_rtl:1.0 $bus
  set_property BUS_TYPE_VLNV xilinx.com:interface:diff_analog_io:1.0 $bus
  set_property INTERFACE_MODE slave $bus
  set_property PHYSICAL_NAME ${port}_n [ipx::add_port_map v_n $bus]
  set_property PHYSICAL_NAME ${port}_p [ipx::add_port_map v_p $bus]
}

set bus [ipx::add_bus_interface B_BRAM $core]
set_property ABSTRACTION_TYPE_VLNV xilinx.com:interface:bram_rtl:1.0 $bus
set_property BUS_TYPE_VLNV xilinx.com:interface:bram:1.0 $bus
set_property INTERFACE_MODE slave $bus
foreach {logical physical} {
  RST  rst
  CLK  clk
  EN   en
  ADDR addr
  DOUT rdata
} {
  set_property PHYSICAL_NAME b_bram_$physical [ipx::add_port_map $logical $bus]
}

set bus [ipx::get_bus_interfaces b_bram_clk]
set parameter [ipx::add_bus_parameter ASSOCIATED_BUSIF $bus]
set_property VALUE B_BRAM $parameter
