---
layout: page
title: Direct memory access
---

Introduction
-----

Two possible approaches to sending data from the FPGA to the CPU:
- use the GP bus to read data from block RAM (BRAM)
- use the HP or ACP bus to write data to DDR3 RAM

The GP bus approach is fast enough for the following applications:
- record relatively short (less than 100k) series of ADC samples with high sample rates (10-125 MSPS)
- continuous transfer of ADC samples to the CPU after some preprocessing (for example, decimation) done on the FPGA and resulting in relatively low sample rates (less than 10 MSPS)

The HP or ACP bus approach can be used for the following applications:
- record long series of ADC samples with high sample rates
- continuous transfer of ADC samples with high sample rates to the CPU

There are several options for controlling the transfer of data from the ADC to the BRAM or DDR3 RAM buffers and accessing the data stored in these buffers.

Options for controlling data transfer from FPGA:
- Xilinx DMA IP cores
- custom DMA IP cores
- custom HDL modules

Options for controlling IP cores and HDL modules from a Linux application running on the CPU:
- `/dev/mem` driver
- UIO driver
- custom Linux driver

Options for accessing data stored in BRAM or DDR3 RAM buffers from a Linux application running on the CPU:
- `/dev/mem` driver
- UIO driver
- custom Linux driver

The applications in this repository use the following combination:
- custom DMA IP cores
- `/dev/mem` driver for controlling IP cores
- custom Linux driver for allocating DDR3 RAM buffers

Custom IP cores
-----

The `axis_ram_writer` module implements the following logic:
- wait until there are 16 entries in the FIFO buffer
- start burst write transfer to send 16 entries from FIFO buffer to RAM
- increment the address counter or reset it when it reaches `cfg_data`

The `axis_ram_reader` module implements the following logic:
- wait until there is space for 16 entries in the FIFO buffer
- start burst read transfer to send 16 entries from RAM to FIFO buffer
- increment the address counter or reset it when it reaches `cfg_data`

The modules have two input ports for dynamically configurable parameters:
- `min_addr` - starting address of the memory buffer
- `cfg_data` - maximum value of the address counter that counts burst transfers (128 bytes per transfer)

The `sts_data` port outputs the current value of the address counter. It can be used to check what memory addresses the modules have already accessed.

The Verilog code of these IP cores can be found in [cores/axis_ram_writer.v](https://github.com/pavel-demin/red-pitaya-notes/tree/master/cores/axis_ram_writer.v) and [cores/axis_ram_reader.v](https://github.com/pavel-demin/red-pitaya-notes/tree/master/cores/axis_ram_reader.v).

Custom Linux driver
-----

The custom Linux driver is used to allocate a memory buffer using contiguous memory allocator (CMA). The `ioctl` function is used to allocate a memory buffer and obtain its physical address. The `mmap` function is used to obtain the virtual address of the memory buffer.

The source code of the custom Linux driver can be found in [patches/cma.c](https://github.com/pavel-demin/red-pitaya-notes/tree/master/patches/cma.c).

Usage examples
-----

The source code of projects using direct memory access can be found at the following links:
- [projects/adc_recorder](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/adc_recorder)
- [projects/adc_recorder_trigger](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/adc_recorder_trigger)
- [projects/adc_test](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/adc_test)
- [projects/dac_player](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/dac_player)
- [projects/sdr_receiver_wide](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_wide)
- [projects/sdr_receiver_wide_122_88](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/sdr_receiver_wide_122_88)
