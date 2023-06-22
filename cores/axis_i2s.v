
`timescale 1 ns / 1 ps

module axis_i2s #
(
  parameter integer AXIS_TDATA_WIDTH = 32
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  // I2S signals
  inout  wire [3:0]                  gpio_data,

  // ALEX signals
  input wire                         alex_flag,
  input wire [3:0]                   alex_data,

  // Slave side
  output wire                        s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid,

  // Master side
  input  wire                        m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  localparam I2S_DATA_WIDTH = AXIS_TDATA_WIDTH / 2;
  localparam CNTR_WIDTH = 6;

  reg [2:0] int_bclk_reg, int_lrclk_reg;
  reg [1:0] int_data_reg;

  reg [1:0] adc_case_reg, adc_case_next;
  reg [CNTR_WIDTH-1:0] adc_cntr_reg, adc_cntr_next;
  reg [I2S_DATA_WIDTH-1:0] adc_data_reg, adc_data_next;
  reg [AXIS_TDATA_WIDTH-1:0] adc_tdata_reg, adc_tdata_next;
  reg adc_tvalid_reg, adc_tvalid_next;

  reg [1:0] dac_case_reg, dac_case_next;
  reg [I2S_DATA_WIDTH-1:0] dac_data_reg, dac_data_next;
  reg [AXIS_TDATA_WIDTH-1:0] dac_tdata_reg, dac_tdata_next;

  wire i2s_bclk, i2s_lrclk, i2s_adc_data, i2s_dac_data;

  wire int_bclk_posedge, int_bclk_negedge, int_lrclk_negedge;

  wire not_alex_flag = ~alex_flag;

  IOBUF buf_bclk (.O(i2s_bclk), .IO(gpio_data[0]), .I(alex_data[0]), .T(not_alex_flag));
  IOBUF buf_adc_data (.O(i2s_adc_data), .IO(gpio_data[1]), .I(alex_data[1]), .T(not_alex_flag));
  IOBUF buf_dac_data (.O(), .IO(gpio_data[2]), .I(alex_flag ? alex_data[2] : i2s_dac_data), .T(1'b0));
  IOBUF buf_lrclk (.O(i2s_lrclk), .IO(gpio_data[3]), .I(alex_data[3]), .T(not_alex_flag));

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_bclk_reg <= 3'd0;
      int_lrclk_reg <= 3'd0;
      int_data_reg <= 2'd0;
      adc_case_reg <= 2'd0;
      adc_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      adc_data_reg <= {(I2S_DATA_WIDTH){1'b0}};
      adc_tdata_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
      adc_tvalid_reg <= 1'b0;
      dac_case_reg <= 2'd0;
      dac_data_reg <= {(I2S_DATA_WIDTH){1'b0}};
      dac_tdata_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
    end
    else
    begin
      int_bclk_reg <= {int_bclk_reg[1:0], i2s_bclk};
      int_lrclk_reg <= {int_lrclk_reg[1:0], i2s_lrclk};
      int_data_reg <= {int_data_reg[0], i2s_adc_data};
      adc_case_reg <= adc_case_next;
      adc_cntr_reg <= adc_cntr_next;
      adc_data_reg <= adc_data_next;
      adc_tdata_reg <= adc_tdata_next;
      adc_tvalid_reg <= adc_tvalid_next;
      dac_case_reg <= dac_case_next;
      dac_data_reg <= dac_data_next;
      dac_tdata_reg <= dac_tdata_next;
    end
  end

  assign int_bclk_posedge = int_bclk_reg[1] & ~int_bclk_reg[2];
  assign int_bclk_negedge = ~int_bclk_reg[1] & int_bclk_reg[2];
  assign int_lrclk_negedge = ~int_lrclk_reg[1] & int_lrclk_reg[2];

  always @*
  begin
    adc_case_next = adc_case_reg;
    adc_cntr_next = adc_cntr_reg;
    adc_data_next = adc_data_reg;
    adc_tdata_next = adc_tdata_reg;
    adc_tvalid_next = adc_tvalid_reg;

    if(int_bclk_posedge & (adc_cntr_reg < I2S_DATA_WIDTH))
    begin
      adc_data_next = {adc_data_reg[I2S_DATA_WIDTH-2:0], int_data_reg[1]};
      adc_cntr_next = adc_cntr_reg + 1'b1;
    end

    if(m_axis_tready & adc_tvalid_reg)
    begin
      adc_tvalid_next = 1'b0;
    end

    case(adc_case_reg)
      2'd0:
      begin
        if(int_lrclk_reg[1] ^ int_lrclk_reg[2])
        begin
          adc_case_next = 2'd1;
        end
      end
      2'd1:
      begin
        if(int_bclk_posedge)
        begin
          adc_cntr_next = {(CNTR_WIDTH){1'b0}};
          adc_case_next = 2'd2;
        end
      end
      2'd2:
      begin
        if(int_bclk_posedge)
        begin
          adc_tvalid_next = ~int_lrclk_reg[1];
          adc_tdata_next = {adc_tdata_reg[I2S_DATA_WIDTH-1:0], adc_data_reg};
          adc_case_next = 2'd0;
        end
      end
    endcase
  end

  always @*
  begin
    dac_case_next = dac_case_reg;
    dac_data_next = dac_data_reg;
    dac_tdata_next = dac_tdata_reg;

    if(int_bclk_negedge)
    begin
      dac_data_next = {dac_data_reg[I2S_DATA_WIDTH-2:0], 1'b0};
    end

    if(int_lrclk_negedge)
    begin
      dac_tdata_next = s_axis_tvalid ? s_axis_tdata : {(AXIS_TDATA_WIDTH){1'b0}};
    end

    case(dac_case_reg)
      2'd0:
      begin
        if(int_lrclk_reg[1] ^ int_lrclk_reg[2])
        begin
          dac_case_next = 2'd1;
        end
      end
      2'd1:
      begin
        if(int_bclk_posedge)
        begin
          dac_case_next = 2'd2;
        end
      end
      2'd2:
      begin
        if(int_bclk_negedge)
        begin
          dac_data_next = int_lrclk_reg[1] ? dac_tdata_reg[I2S_DATA_WIDTH-1:0] : dac_tdata_reg[AXIS_TDATA_WIDTH-1:I2S_DATA_WIDTH];
          dac_case_next = 2'd0;
        end
      end
    endcase
  end

  assign i2s_dac_data = dac_data_reg[I2S_DATA_WIDTH-1];
  assign s_axis_tready = int_lrclk_negedge;
  assign m_axis_tdata = adc_tdata_reg;
  assign m_axis_tvalid = adc_tvalid_reg;

endmodule
