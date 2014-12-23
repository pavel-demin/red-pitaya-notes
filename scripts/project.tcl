if {[catch {

  set project_name [lindex $argv 0]

  set part_name [lindex $argv 1]

  create_project -part $part_name $project_name

  set bd_path "$project_name.srcs/sources_1/bd/system"

  create_bd_design system

  source cfg/system_bd.tcl

  generate_target all [get_files $bd_path/system.bd]
  make_wrapper -files [get_files $bd_path/system.bd] -top

  add_files -norecurse $bd_path/hdl/system_wrapper.v

  add_files -norecurse [glob $project_name/*.v]
  add_files -norecurse -fileset constrs_1 [glob cfg/*.xdc]

  set_property VERILOG_DEFINE {TOOL_VIVADO} [current_fileset]

  set_property STRATEGY Flow_PerfOptimized_High [get_runs synth_1]
  set_property STRATEGY Performance_NetDelay_high [get_runs impl_1]

  close_project

} result]} {
  puts "** ERROR: $result"
  exit 1
}

exit

