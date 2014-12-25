# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  set Component_Name  [  ipgui::add_param $IPINST -name "Component_Name" -display_name {Component Name}]
  set_property tooltip {Component Name} ${Component_Name}
  #Adding Page
  set Page_0  [  ipgui::add_page $IPINST -name "Page 0" -display_name {Page 0}]
  set_property tooltip {Page 0} ${Page_0}
  set AXIS_TDATA_WIDTH  [  ipgui::add_param $IPINST -name "AXIS_TDATA_WIDTH" -parent ${Page_0} -display_name {AXIS TDATA WIDTH}]
  set_property tooltip {Width of the M_AXIS data bus.} ${AXIS_TDATA_WIDTH}
  set PACKET_SIZE  [  ipgui::add_param $IPINST -name "PACKET_SIZE" -parent ${Page_0} -display_name {PACKET SIZE}]
  set_property tooltip {Size of the packet.} ${PACKET_SIZE}


}

proc update_PARAM_VALUE.PACKET_SIZE { PARAM_VALUE.PACKET_SIZE } {
	# Procedure called to update PACKET_SIZE when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.PACKET_SIZE { PARAM_VALUE.PACKET_SIZE } {
	# Procedure called to validate PACKET_SIZE
	return true
}

proc update_PARAM_VALUE.AXIS_TDATA_WIDTH { PARAM_VALUE.AXIS_TDATA_WIDTH } {
	# Procedure called to update AXIS_TDATA_WIDTH when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.AXIS_TDATA_WIDTH { PARAM_VALUE.AXIS_TDATA_WIDTH } {
	# Procedure called to validate AXIS_TDATA_WIDTH
	return true
}


proc update_MODELPARAM_VALUE.PACKET_SIZE { MODELPARAM_VALUE.PACKET_SIZE PARAM_VALUE.PACKET_SIZE } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.PACKET_SIZE}] ${MODELPARAM_VALUE.PACKET_SIZE}
}

proc update_MODELPARAM_VALUE.AXIS_TDATA_WIDTH { MODELPARAM_VALUE.AXIS_TDATA_WIDTH PARAM_VALUE.AXIS_TDATA_WIDTH } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.AXIS_TDATA_WIDTH}] ${MODELPARAM_VALUE.AXIS_TDATA_WIDTH}
}

