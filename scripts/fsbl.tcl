if {[catch {

  set project_name [lindex $argv 0]

  set proc_name [lindex $argv 1]

  file mkdir tmp/hw
  file copy -force $project_name.hwdef tmp/hw/$project_name.hdf

  open_hw_design tmp/hw/$project_name.hdf
  create_sw_design -proc $proc_name -os standalone fsbl

  add_library xilffs
  add_library xilrsa

  generate_app -proc $proc_name -app zynq_fsbl -dir tmp/sw -compile

  close_hw_design [current_hw_design]

  file copy -force tmp/sw/executable.elf $project_name.elf

} result]} {
  puts "** ERROR: $result"
  exit 1
}

exit

