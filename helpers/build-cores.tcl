set cores [list \
  axi_axis_reader_v1_0 axi_axis_writer_v1_0 axi_bram_reader_v1_0 \
  axi_bram_writer_v1_0 axi_cfg_register_v1_0 axi_sts_register_v1_0 \
  axis_accumulator_v1_0 axis_adder_v1_0 axis_alex_v1_0 axis_averager_v1_0 \
  axis_bram_reader_v1_0 axis_bram_writer_v1_0 axis_constant_v1_0 \
  axis_counter_v1_0 axis_decimator_v1_0 axis_fifo_v1_0 \
  axis_gate_controller_v1_0 axis_gpio_reader_v1_0 axis_histogram_v1_0 \
  axis_i2s_v1_0 axis_iir_filter_v1_0 axis_interpolator_v1_0 axis_keyer_v1_0 \
  axis_lfsr_v1_0 axis_negator_v1_0 axis_oscilloscope_v1_0 axis_packetizer_v1_0 \
  axis_phase_generator_v1_0 axis_pps_counter_v1_0 axis_pulse_generator_v1_0 \
  axis_pulse_height_analyzer_v1_0 axis_ram_writer_v1_0 \
  axis_red_pitaya_adc_v2_0 axis_red_pitaya_dac_v1_0 axis_stepper_v1_0 \
  axis_tagger_v1_0 axis_timer_v1_0 axis_trigger_v1_0 axis_validator_v1_0 \
  axis_variable_v1_0 axis_variant_v1_0 axis_zeroer_v1_0 dna_reader_v1_0 \
  gpio_debouncer_v1_0 port_selector_v1_0 port_slicer_v1_0 pulse_generator_v1_0 \
  shift_register_v1_0 \
]

set part_name xc7z010clg400-1

foreach core_name $cores {
  set argv [list $core_name $part_name]
  source scripts/core.tcl
}
