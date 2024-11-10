# AXI4 hub

## Requirements

All applications in this repository have a structure similar to the one shown in the following diagram:

![Application structure](/img/application-structure.png)

To control, monitor and communicate with all parts of the applications, the following items are required:

- configuration registers
- status registers
- AXI4-Stream interfaces
- BRAM interfaces

## Hub interface

The hub interface consists of all required registers and interfaces connected to the different parts of the applications and an AXI4 slave interface used to communicate with the CPU.

The corresponding Verilog code can be found in [cores/axi_hub.v](https://github.com/pavel-demin/red-pitaya-notes/blob/master/cores/axi_hub.v).

## Addresses

Bits 24-26 of the address are used to select one of the hub ports:

| hub port        | hub address |
| --------------- | ----------- |
| config register | 0           |
| status register | 1           |
| interface 0     | 2           |
| interface 1     | 3           |
| interface 2     | 4           |
| interface 3     | 5           |
| interface 4     | 6           |
| interface 5     | 7           |

## Usage examples

A basic project with the hub interface, ADC interface, and DAC interface is shown in the following diagram:

![Template project](/img/template-project.png)

This template project can be used as a starting point for projects requiring ADC, DAC and hub interface. The Tcl code of this project can be found in [projects/template](https://github.com/pavel-demin/red-pitaya-notes/tree/master/projects/template).
