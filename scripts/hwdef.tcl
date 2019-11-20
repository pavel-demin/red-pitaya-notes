
set project_name [lindex $argv 0]

open_project tmp/$project_name.xpr

write_hw_platform -fixed -force -file tmp/$project_name.xsa

close_project
