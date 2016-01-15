
set project_name [lindex $argv 0]

set proc_name [lindex $argv 1]

set hard_path tmp/$project_name.hard
set fsbl_path tmp/$project_name.fsbl

file mkdir $hard_path
file copy -force tmp/$project_name.hwdef $hard_path/$project_name.hdf

open_hw_design $hard_path/$project_name.hdf
create_sw_design -proc $proc_name -os standalone fsbl

add_library xilffs
add_library xilrsa

generate_app -proc $proc_name -app zynq_fsbl -dir $fsbl_path -compile

close_hw_design [current_hw_design]
