if {[catch {

  set project_name [lindex $argv 0]

  open_project $project_name.xpr

  if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
   launch_runs impl_1 -to_step route_design
    wait_on_run impl_1
  }

  open_run [get_runs impl_1]

  write_bitstream -force -file $project_name.bit

  close_project

} result]} {
  puts "** ERROR: $result"
  exit 1
}

exit

