
`timescale 1 ns / 1 ps

module axis_misc_reader #
(
  parameter integer S_AXIS_TDATA_WIDTH = 40,
  parameter integer M_AXIS_TDATA_WIDTH = 32,
  parameter integer MISC_WIDTH = 8
)
(
  // System signals
  input  wire                          aclk,
  input  wire                          aresetn,

  output wire [MISC_WIDTH-1:0]         misc_data,

  // Slave side
  output wire                          s_axis_tready,
  input  wire [S_AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                          s_axis_tvalid,

  // Master side
  input  wire                          m_axis_tready,
  output wire [M_AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                          m_axis_tvalid
);

  reg int_enbl_reg;
  reg [MISC_WIDTH-1:0] int_misc_reg;

  reg [39:0] pulse_counter;
  reg key_latch;
  reg [53:0] uart_buffer;
  reg [7:0] uart_pos;
  reg [5:0] uart_prescaler;

  reg [39:0] reflect_buffer;
  reg [7:0] reflect_pos;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_enbl_reg <= 1'b0;
      pulse_counter <= 40'd0;
      key_latch <= 1'b0;
      uart_pos <= 8'd0;
      uart_buffer <= 54'd0;
      uart_prescaler <= 6'd0;
      reflect_pos <= 8'd0;
    end
    else
    begin
      int_enbl_reg <= 1'b1;
      if(s_axis_tvalid & s_axis_tready)
      begin
        int_misc_reg[MISC_WIDTH-1:3] <= s_axis_tdata[S_AXIS_TDATA_WIDTH-1:S_AXIS_TDATA_WIDTH-MISC_WIDTH+3];

        int_misc_reg[0] <= uart_buffer[uart_pos];
        int_misc_reg[1] <= reflect_buffer[reflect_pos];
        int_misc_reg[2] <= reflect_pos != 0;

        // fast metadata reflect
        if(s_axis_tdata[S_AXIS_TDATA_WIDTH-1] & (~key_latch))
        begin
          // every pulse
          reflect_pos <= 8'd0;
          reflect_buffer <= pulse_counter;

          // increment pulse counter
          pulse_counter <= pulse_counter + 1'b1;
        end
        else
        begin
          reflect_pos <= reflect_pos < 39 ? reflect_pos + 1'b1 : reflect_pos;
        end

        if(s_axis_tdata[S_AXIS_TDATA_WIDTH-1] & (~key_latch) & (pulse_counter[3:0] == 4'h0))
        begin
          // beginning of every 16th pulse
          // copy data to uart buffer

          uart_buffer[0] <= 1'b0; // start bit
          uart_buffer[7:1] <= pulse_counter[10:4];
          uart_buffer[8] <= 1'b1; // protocol signalization - first byte
          uart_buffer[9] <= 1'b1; // stop bit
          uart_buffer[10] <= 1'b1; // pad bit
          uart_buffer[11] <= 1'b0; // start bit
          uart_buffer[18:12] <= pulse_counter[17:11];
          uart_buffer[20] <= 1'b1; // stop bit
          uart_buffer[21] <= 1'b1; // pad bit
          uart_buffer[22] <= 1'b0; // start bit
          uart_buffer[29:23] <= pulse_counter[24:18];
          uart_buffer[31] <= 1'b1; // stop bit
          uart_buffer[32] <= 1'b1; // pad bit
          uart_buffer[33] <= 1'b0; // start bit
          uart_buffer[40:34] <= pulse_counter[32:25];
          uart_buffer[42] <= 1'b1; // stop bit
          uart_buffer[43] <= 1'b1; // pad bit
          uart_buffer[44] <= 1'b0; // start bit
          uart_buffer[51:45] <= pulse_counter[39:33];
          uart_buffer[53] <= 1'b1; // stop bit

          // reset uart transmitter position
          uart_pos <= 8'd0;
          uart_prescaler <= 6'd0;
        end
        else
        begin
          // not the beginning of pulse - advance UART.
          // This will truncate the transmission if the next pulse is received too early;
          // the caller is responsible to sanitize this.
          if(uart_prescaler == 62)
          begin
            uart_pos <= uart_pos < 53 ? uart_pos + 1'b1 : uart_pos; // transmit stop bit forever after the end of the transmission
          end

          uart_prescaler <= uart_prescaler < 62 ? uart_prescaler + 1'b1 : 6'd0;
        end

        key_latch <= s_axis_tdata[S_AXIS_TDATA_WIDTH-1];
      end
    end
  end

  assign s_axis_tready = int_enbl_reg & m_axis_tready;

  assign misc_data = int_misc_reg;

  assign m_axis_tdata = s_axis_tdata[M_AXIS_TDATA_WIDTH-1:0];
  assign m_axis_tvalid = int_enbl_reg & s_axis_tvalid;

endmodule
