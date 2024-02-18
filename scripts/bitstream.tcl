
set project_name [lindex $argv 0]

open_project tmp/$project_name.xpr

if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
  launch_runs impl_1
  wait_on_run impl_1
}

open_run [get_runs impl_1]

set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]

write_bitstream -force -file tmp/$project_name.bit

close_project
