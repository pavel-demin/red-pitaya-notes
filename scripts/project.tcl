if {[catch {

  set project_name [lindex $argv 0]

  set part_name [lindex $argv 1]

  create_project -part $part_name $project_name

  set_property ip_repo_paths lib [current_project]

  set bd_path "$project_name.srcs/sources_1/bd/system"

  create_bd_design system

  source $project_name/block_design.tcl

  generate_target all [get_files $bd_path/system.bd]
  make_wrapper -files [get_files $bd_path/system.bd] -top

  add_files -norecurse $bd_path/hdl/system_wrapper.v

  set files [glob -nocomplain $project_name/*.v]
  if {[llength $files] > 0} {
    add_files -norecurse $files
  }

  set files [glob -nocomplain cfg/*.xdc]
  if {[llength $files] > 0} {
    add_files -norecurse -fileset constrs_1 $files
  }

  set_property STRATEGY Flow_PerfOptimized_High [get_runs synth_1]
  set_property STRATEGY Performance_NetDelay_high [get_runs impl_1]

  close_project

} result]} {
  puts "** ERROR: $result"
  exit 1
}

exit

