
set project_name [lindex $argv 0]

set proc_name [lindex $argv 1]

set hard_path tmp/$project_name.hard
set fsbl_path tmp/$project_name.fsbl

file mkdir $hard_path
file copy -force tmp/$project_name.xsa $hard_path/$project_name.xsa

hsi open_hw_design $hard_path/$project_name.xsa
hsi create_sw_design -proc $proc_name -os standalone fsbl

hsi add_library xilffs
hsi add_library xilrsa

hsi generate_app -proc $proc_name -app zynq_fsbl -dir $fsbl_path -compile

hsi close_hw_design [hsi current_hw_design]
