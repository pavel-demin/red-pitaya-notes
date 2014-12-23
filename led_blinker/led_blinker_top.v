module led_blinker_top
  (
   // PS connections
   inout [54-1: 0]  FIXED_IO_mio ,
   inout            FIXED_IO_ps_clk ,
   inout            FIXED_IO_ps_porb ,
   inout            FIXED_IO_ps_srstb ,
   inout            FIXED_IO_ddr_vrn ,
   inout            FIXED_IO_ddr_vrp ,
   inout [15-1: 0]  DDR_addr ,
   inout [ 3-1: 0]  DDR_ba ,
   inout            DDR_cas_n ,
   inout            DDR_ck_n ,
   inout            DDR_ck_p ,
   inout            DDR_cke ,
   inout            DDR_cs_n ,
   inout [ 4-1: 0]  DDR_dm ,
   inout [32-1: 0]  DDR_dq ,
   inout [ 4-1: 0]  DDR_dqs_n ,
   inout [ 4-1: 0]  DDR_dqs_p ,
   inout            DDR_odt ,
   inout            DDR_ras_n ,
   inout            DDR_reset_n ,
   inout            DDR_we_n ,

   // Red Pitaya periphery
  
   // ADC
   input [16-1: 2]  adc_dat_a_i , // ADC CH1
   input [16-1: 2]  adc_dat_b_i , // ADC CH2
   input            adc_clk_p_i , // ADC data clock
   input            adc_clk_n_i , // ADC data clock
   output [ 2-1: 0] adc_clk_o , // optional ADC clock source
   output           adc_cdcs_o , // ADC clock duty cycle stabilizer
  
   // DAC
   output [14-1: 0] dac_dat_o , // DAC combined data
   output           dac_wrt_o , // DAC write
   output           dac_sel_o , // DAC channel select
   output           dac_clk_o , // DAC clock
   output           dac_rst_o , // DAC reset
  
   // PWM DAC
   output [ 4-1: 0] dac_pwm_o , // serial PWM DAC

   // XADC
   input [ 5-1: 0]  vinp_i , // voltages p
   input [ 5-1: 0]  vinn_i , // voltages n

   // Expansion connector
   inout [ 8-1: 0]  exp_p_io ,
   inout [ 8-1: 0]  exp_n_io ,

   // SATA connector
   output [ 2-1: 0] daisy_p_o , // line 1 is clock capable
   output [ 2-1: 0] daisy_n_o ,
   input [ 2-1: 0]  daisy_p_i , // line 1 is clock capable
   input [ 2-1: 0]  daisy_n_i ,

   // LED
   output [ 8-1: 0] led_o       
   );

   //------------------------------------------------------------------------------
   //
   //  Connections to PS

   wire [  4-1: 0]  fclk               ; //[0]-125MHz, [1]-250MHz, [2]-50MHz, [3]-200MHz
   wire [  4-1: 0]  frstn              ;

   led_blinker_ps i_ps
     (
      .FIXED_IO_mio       (  FIXED_IO_mio                ),
      .FIXED_IO_ps_clk    (  FIXED_IO_ps_clk             ),
      .FIXED_IO_ps_porb   (  FIXED_IO_ps_porb            ),
      .FIXED_IO_ps_srstb  (  FIXED_IO_ps_srstb           ),
      .FIXED_IO_ddr_vrn   (  FIXED_IO_ddr_vrn            ),
      .FIXED_IO_ddr_vrp   (  FIXED_IO_ddr_vrp            ),
      .DDR_addr           (  DDR_addr                    ),
      .DDR_ba             (  DDR_ba                      ),
      .DDR_cas_n          (  DDR_cas_n                   ),
      .DDR_ck_n           (  DDR_ck_n                    ),
      .DDR_ck_p           (  DDR_ck_p                    ),
      .DDR_cke            (  DDR_cke                     ),
      .DDR_cs_n           (  DDR_cs_n                    ),
      .DDR_dm             (  DDR_dm                      ),
      .DDR_dq             (  DDR_dq                      ),
      .DDR_dqs_n          (  DDR_dqs_n                   ),
      .DDR_dqs_p          (  DDR_dqs_p                   ),
      .DDR_odt            (  DDR_odt                     ),
      .DDR_ras_n          (  DDR_ras_n                   ),
      .DDR_reset_n        (  DDR_reset_n                 ),
      .DDR_we_n           (  DDR_we_n                    ),

      .fclk_clk_o      (  fclk               ),
      .fclk_rstn_o     (  frstn              ),

      // SPI master
      .spi_ss_o        (                     ),  // select slave 0
      .spi_ss1_o       (                     ),  // select slave 1
      .spi_ss2_o       (                     ),  // select slave 2
      .spi_sclk_o      (                     ),  // serial clock
      .spi_mosi_o      (                     ),  // master out slave in
      .spi_miso_i      (  1'b0               ),  // master in slave out

      // SPI slave
      .spi_ss_i        (  1'b1               ),  // slave selected
      .spi_sclk_i      (  1'b0               ),  // serial clock
      .spi_mosi_i      (  1'b0               ),  // master out slave in
      .spi_miso_o      (                     )   // master in slave out
      );

   wire             adc_clk_in ;
   wire             adc_clk;

   IBUFDS i_clk ( .I(adc_clk_p_i), .IB(adc_clk_n_i), .O(adc_clk_in));  // differential clock input
   BUFG i_adc_buf  (.O(adc_clk), .I(adc_clk_in)); // use global clock buffer

   // ADC clock duty cycle stabilizer is enabled
   assign adc_cdcs_o = 1'b1 ;

   reg [31:0] counter;
   always @(posedge adc_clk) counter <= counter + 32'd1;
   assign led_o[0] = counter[26];

endmodule

