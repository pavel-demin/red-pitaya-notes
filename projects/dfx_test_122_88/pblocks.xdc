create_pblock pblock_rp1
resize_pblock pblock_rp1 -add {SLICE_X26Y100:SLICE_X49Y149 DSP48_X2Y40:DSP48_X2Y59 RAMB18_X2Y40:RAMB18_X2Y59 RAMB36_X2Y20:RAMB36_X2Y29}
add_cells_to_pblock pblock_rp1 [get_cells system_i/rp1]
set_property SNAPPING_MODE ON [get_pblocks pblock_rp1]
