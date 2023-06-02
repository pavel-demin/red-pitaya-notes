set cores [list \
  axi_hub axis_accumulator axis_adder axis_alex axis_averager \
  axis_bram_reader axis_bram_writer axis_constant axis_counter axis_decimator \
  axis_fifo axis_fifo axis_gate_controller axis_gpio_reader axis_histogram \
  axis_i2s axis_iir_filter axis_interpolator axis_keyer axis_lfsr \
  axis_maxabs_finder axis_negator axis_oscilloscope axis_packetizer axis_pdm \
  axis_phase_generator axis_pps_counter axis_pulse_generator \
  axis_pulse_height_analyzer axis_ram_writer axis_red_pitaya_adc \
  axis_red_pitaya_dac axis_selector axis_stepper axis_tagger axis_timer \
  axis_trigger axis_validator axis_variable axis_variant axis_zeroer \
  dna_reader dsp48 edge_detector gpio_debouncer port_selector port_slicer \
  pulse_generator shift_register xadc_bram \
]

set part_name xc7z010clg400-1

foreach core_name $cores {
  set argv [list $core_name $part_name]
  source scripts/core.tcl
}
