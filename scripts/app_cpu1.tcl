
set project_name [lindex $argv 0]

set proc_name ps7_cortexa9_1

set hard_path tmp/$project_name.hard
set cpu1_path tmp/$project_name.cpu1

file mkdir $hard_path
file copy -force tmp/$project_name.hwdef $hard_path/$project_name.hdf

open_hw_design $hard_path/$project_name.hdf
create_sw_design -proc $proc_name -os standalone system

set_property CONFIG.stdin {none} [get_os]
set_property CONFIG.stdout {none} [get_os]

set_property CONFIG.extra_compiler_flags { -g -DUSE_AMP=1 -DSTDOUT_REDIR=1} [get_sw_processor]

generate_bsp -proc $proc_name -dir $cpu1_path/app_cpu1_bsp

close_hw_design [current_hw_design]
