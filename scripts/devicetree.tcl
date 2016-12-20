
set project_name [lindex $argv 0]

set proc_name [lindex $argv 1]

set repo_path [lindex $argv 2]

set boot_args {console=ttyPS0,115200 root=/dev/mmcblk0p2 ro rootfstype=ext4 earlyprintk rootwait}

set hard_path tmp/$project_name.hard
set tree_path tmp/$project_name.tree

file mkdir $hard_path
file copy -force tmp/$project_name.hwdef $hard_path/$project_name.hdf

set_repo_path $repo_path

open_hw_design $hard_path/$project_name.hdf
create_sw_design -proc $proc_name -os device_tree devicetree

set_property CONFIG.kernel_version {2016.4} [get_os]
set_property CONFIG.bootargs $boot_args [get_os]

generate_bsp -dir $tree_path

close_sw_design [current_sw_design]
close_hw_design [current_hw_design]
