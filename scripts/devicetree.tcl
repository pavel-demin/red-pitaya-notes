if {[catch {

  set project_name [lindex $argv 0]

  set proc_name [lindex $argv 1]

  set repo_path [lindex $argv 2]

  set boot_args {console=ttyPS0,115200 root=/dev/mmcblk0p2 ro rootfstype=ext4 earlyprintk rootwait}

  file mkdir tmp/hw
  file copy -force $project_name.hwdef tmp/hw/$project_name.hdf

  set_repo_path $repo_path

  open_hw_design tmp/hw/$project_name.hdf
  create_sw_design -proc $proc_name -os device_tree devicetree

  set_property CONFIG.kernel_version {2014.3} [get_os]
  set_property CONFIG.bootargs $boot_args [get_os]

  generate_bsp -dir tmp/dt

  close_sw_design [current_sw_design]
  close_hw_design [current_hw_design]

  file copy -force tmp/dt/system.dts $project_name.dts
  file copy -force tmp/dt/ps.dtsi ps.dtsi

} result]} {
  puts "** ERROR: $result"
  exit 1
}

exit

