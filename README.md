# Notes on Web-888 support

This fork is designed to support the Red Pitaya Notes project on Web-888 hardware. The Web-888 uses a Zynq7010 chip, which is largely compatible with Red Pitaya on the receive side. The ADC clock is driven by the Si5351, which has tuning capabilities. In the bootloader, the Si5351 is configured to output 125 MHz to maximize compatibility with 7010-based Red Pitaya boards.

Although the Web-888 lacks a DAC, it offers a superior RF frontend. This makes many projects in Red Pitaya Notes particularly interesting.

For more information about the Web-888, visit http://www.rx-888.com/web/.

# Red Pitaya Notes

Notes on the Red Pitaya Open Source Instrument

http://pavel-demin.github.io/red-pitaya-notes/
