---
layout: page
title: Buffers for AXI4, AXI4-Lite and AXI4-Stream interfaces
---

Interesting links
-----

Some interesting links on implementation of buffers and AXI4 interfaces:

 - [Building a custom yet functional AXI-lite slave](https://zipcpu.com/blog/2019/01/12/demoaxilite.html)

 - [Building a Skid Buffer for AXI processing](https://zipcpu.com/blog/2019/05/22/skidbuffer.html)

 - [Pipeline Skid Buffer](http://fpgacpu.ca/fpga/Pipeline_Skid_Buffer.html)

Requirements
-----

Since the AXI4 protocol specification requires that there are no combinational paths between input and output signals, it is useful to have reusable modules that would add the registers to the output signals of AXI4 interfaces.

To achieve good performance, these modules should have low resource usage and minimal latency.

Here are the main signals of the read address and read data parts of the AXI4 slave interface:
{% highlight Verilog %}
input  wire [ADDR_WIDTH-1:0] s_axi_araddr,  // AXI4-Lite slave: Read address
input  wire                  s_axi_arvalid, // AXI4-Lite slave: Read address valid
output wire                  s_axi_arready, // AXI4-Lite slave: Read address ready

output wire [DATA_WIDTH-1:0] s_axi_rdata,   // AXI4-Lite slave: Read data
output wire                  s_axi_rvalid,  // AXI4-Lite slave: Read data valid
input  wire                  s_axi_rready   // AXI4-Lite slave: Read data ready
{% endhighlight %}

In the read address part of the interface, the `arready` signal should have a register. In the read data part of the interface, the `rdata` and `rvalid` signals should have registers. So, two types of buffers are needed, one with one register on the data input side and one with two registers on the data output side as shown in the diagrams below:

![Two types of interface buffers]({% link img/interface-buffers.png %})

If an interface buffer with registers on both sides is required, then these two types of buffers can be chained together to create such a buffer.

Input buffer
-----

The `in_ready` signal of the input buffer should have a register and this buffer should have the following behavior:
 - `in_ready` register is set to high during reset
 - when `in_ready` is high, `in_valid` and `in_data` are directly connected to `out_valid` and `out_data`
 - when `out_valid` is high and `out_ready` is low, `in_ready` becomes low, `out_valid` and `out_data` should output the last valid data until `out_ready` goes high

A circuit implementing this behavior is shown in the following diagram:

![Input buffer]({% link img/input-buffer.png %})

The corresponding Verilog code can be found in [modules/input_buffer.v](https://github.com/pavel-demin/red-pitaya-notes/tree/master/modules/input_buffer.v).

Output buffer
-----

The `out_valid` and `out_data` signals of the output buffer should have registers and these registers should be updated while `out_ready` is high or `out_valid` is low.

A circuit implementing this behavior is shown in the following diagram:

![Output buffer]({% link img/output-buffer.png %})

The corresponding Verilog code can be found in [modules/output_buffer.v](https://github.com/pavel-demin/red-pitaya-notes/tree/master/modules/output_buffer.v).

Since the `in_ready` signal is used to enable the data register, it can also be used to enable data registers outside the output buffer module. This feature can be useful when controlling a pipeline that provides enable inputs for its internal registers, like for example the internal registers in [DSP48E1](https://docs.xilinx.com/v/u/en-US/ug479_7Series_DSP48E1) as shown in the diagram below:

![Output buffers and DSP48]({% link img/output-buffers-dsp48.png %})

Usage examples
-----

The Verilog code of the modules that use these input and output buffers can be found at the following links:

 - [AXI4 Hub](https://github.com/pavel-demin/red-pitaya-notes/blob/master/cores/axi_hub.v)

 - [AXI4-Stream IIR Filter](https://github.com/pavel-demin/red-pitaya-notes/blob/master/cores/axis_iir_filter.v)
