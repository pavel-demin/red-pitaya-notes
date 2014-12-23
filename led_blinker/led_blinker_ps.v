module led_blinker_ps
  (
   // PS peripherals
   inout [ 54-1: 0] FIXED_IO_mio ,
   inout            FIXED_IO_ps_clk ,
   inout            FIXED_IO_ps_porb ,
   inout            FIXED_IO_ps_srstb ,
   inout            FIXED_IO_ddr_vrn ,
   inout            FIXED_IO_ddr_vrp ,
   inout [ 15-1: 0] DDR_addr ,
   inout [ 3-1: 0]  DDR_ba ,
   inout            DDR_cas_n ,
   inout            DDR_ck_n ,
   inout            DDR_ck_p ,
   inout            DDR_cke ,
   inout            DDR_cs_n ,
   inout [ 4-1: 0]  DDR_dm ,
   inout [ 32-1: 0] DDR_dq ,
   inout [ 4-1: 0]  DDR_dqs_n ,
   inout [ 4-1: 0]  DDR_dqs_p ,
   inout            DDR_odt ,
   inout            DDR_ras_n ,
   inout            DDR_reset_n ,
   inout            DDR_we_n ,

   output [ 4-1: 0] fclk_clk_o ,
   output [ 4-1: 0] fclk_rstn_o ,

   // SPI master
   output           spi_ss_o , // select slave 0
   output           spi_ss1_o , // select slave 1
   output           spi_ss2_o , // select slave 2
   output           spi_sclk_o , // serial clock
   output           spi_mosi_o , // master out slave in
   input            spi_miso_i , // master in slave out

   // SPI slave
   input            spi_ss_i , // slave selected
   input            spi_sclk_i , // serial clock
   input            spi_mosi_i , // master out slave in
   output           spi_miso_o            // master in slave out
   );

   wire [  4-1: 0]  fclk_clk             ;
   wire [  4-1: 0]  fclk_rstn            ;

   wire             gp0_maxi_arvalid     ;
   wire             gp0_maxi_awvalid     ;
   wire             gp0_maxi_bready      ;
   wire             gp0_maxi_rready      ;
   wire             gp0_maxi_wlast       ;
   wire             gp0_maxi_wvalid      ;
   wire [ 12-1: 0]  gp0_maxi_arid        ;
   wire [ 12-1: 0]  gp0_maxi_awid        ;
   wire [ 12-1: 0]  gp0_maxi_wid         ;
   wire [  2-1: 0]  gp0_maxi_arburst     ;
   wire [  2-1: 0]  gp0_maxi_arlock      ;
   wire [  3-1: 0]  gp0_maxi_arsize      ;
   wire [  2-1: 0]  gp0_maxi_awburst     ;
   wire [  2-1: 0]  gp0_maxi_awlock      ;
   wire [  3-1: 0]  gp0_maxi_awsize      ;
   wire [  3-1: 0]  gp0_maxi_arprot      ;
   wire [  3-1: 0]  gp0_maxi_awprot      ;
   wire [ 32-1: 0]  gp0_maxi_araddr      ;
   wire [ 32-1: 0]  gp0_maxi_awaddr      ;
   wire [ 32-1: 0]  gp0_maxi_wdata       ;
   wire [  4-1: 0]  gp0_maxi_arcache     ;
   wire [  4-1: 0]  gp0_maxi_arlen       ;
   wire [  4-1: 0]  gp0_maxi_arqos       ;
   wire [  4-1: 0]  gp0_maxi_awcache     ;
   wire [  4-1: 0]  gp0_maxi_awlen       ;
   wire [  4-1: 0]  gp0_maxi_awqos       ;
   wire [  4-1: 0]  gp0_maxi_wstrb       ;
   wire             gp0_maxi_aclk        ;
   wire             gp0_maxi_arready     ;
   wire             gp0_maxi_awready     ;
   wire             gp0_maxi_bvalid      ;
   wire             gp0_maxi_rlast       ;
   wire             gp0_maxi_rvalid      ;
   wire             gp0_maxi_wready      ;
   wire [ 12-1: 0]  gp0_maxi_bid         ;
   wire [ 12-1: 0]  gp0_maxi_rid         ;
   wire [  2-1: 0]  gp0_maxi_bresp       ;
   wire [  2-1: 0]  gp0_maxi_rresp       ;
   wire [ 32-1: 0]  gp0_maxi_rdata       ;
   wire             gp0_maxi_arstn       ;

   assign fclk_rstn_o    = fclk_rstn      ;
   assign gp0_maxi_aclk  = fclk_clk_o[0]  ;

   BUFG i_fclk0_buf  (.O(fclk_clk_o[0]), .I(fclk_clk[0]));
   BUFG i_fclk1_buf  (.O(fclk_clk_o[1]), .I(fclk_clk[1]));
   BUFG i_fclk2_buf  (.O(fclk_clk_o[2]), .I(fclk_clk[2]));
   BUFG i_fclk3_buf  (.O(fclk_clk_o[3]), .I(fclk_clk[3]));

   system_wrapper system_i
     (
      // MIO
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

      // FCLKs
      .FCLK_CLK0          (  fclk_clk[0]                 ),  // out
      .FCLK_CLK1          (  fclk_clk[1]                 ),  // out
      .FCLK_CLK2          (  fclk_clk[2]                 ),  // out
      .FCLK_CLK3          (  fclk_clk[3]                 ),  // out
      .FCLK_RESET0_N      (  fclk_rstn[0]                ),  // out
      .FCLK_RESET1_N      (  fclk_rstn[1]                ),  // out
      .FCLK_RESET2_N      (  fclk_rstn[2]                ),  // out
      .FCLK_RESET3_N      (  fclk_rstn[3]                ),  // out

      // GP0
      .M_AXI_GP0_arvalid  (  gp0_maxi_arvalid            ),  // out
      .M_AXI_GP0_awvalid  (  gp0_maxi_awvalid            ),  // out
      .M_AXI_GP0_bready   (  gp0_maxi_bready             ),  // out
      .M_AXI_GP0_rready   (  gp0_maxi_rready             ),  // out
      .M_AXI_GP0_wlast    (  gp0_maxi_wlast              ),  // out
      .M_AXI_GP0_wvalid   (  gp0_maxi_wvalid             ),  // out
      .M_AXI_GP0_arid     (  gp0_maxi_arid               ),  // out 12
      .M_AXI_GP0_awid     (  gp0_maxi_awid               ),  // out 12
      .M_AXI_GP0_wid      (  gp0_maxi_wid                ),  // out 12
      .M_AXI_GP0_arburst  (  gp0_maxi_arburst            ),  // out 2
      .M_AXI_GP0_arlock   (  gp0_maxi_arlock             ),  // out 2
      .M_AXI_GP0_arsize   (  gp0_maxi_arsize             ),  // out 3
      .M_AXI_GP0_awburst  (  gp0_maxi_awburst            ),  // out 2
      .M_AXI_GP0_awlock   (  gp0_maxi_awlock             ),  // out 2
      .M_AXI_GP0_awsize   (  gp0_maxi_awsize             ),  // out 3
      .M_AXI_GP0_arprot   (  gp0_maxi_arprot             ),  // out 3
      .M_AXI_GP0_awprot   (  gp0_maxi_awprot             ),  // out 3
      .M_AXI_GP0_araddr   (  gp0_maxi_araddr             ),  // out 32
      .M_AXI_GP0_awaddr   (  gp0_maxi_awaddr             ),  // out 32
      .M_AXI_GP0_wdata    (  gp0_maxi_wdata              ),  // out 32
      .M_AXI_GP0_arcache  (  gp0_maxi_arcache            ),  // out 4
      .M_AXI_GP0_arlen    (  gp0_maxi_arlen              ),  // out 4
      .M_AXI_GP0_arqos    (  gp0_maxi_arqos              ),  // out 4
      .M_AXI_GP0_awcache  (  gp0_maxi_awcache            ),  // out 4
      .M_AXI_GP0_awlen    (  gp0_maxi_awlen              ),  // out 4
      .M_AXI_GP0_awqos    (  gp0_maxi_awqos              ),  // out 4
      .M_AXI_GP0_wstrb    (  gp0_maxi_wstrb              ),  // out 4
      .M_AXI_GP0_arready  (  gp0_maxi_arready            ),  // in
      .M_AXI_GP0_awready  (  gp0_maxi_awready            ),  // in
      .M_AXI_GP0_bvalid   (  gp0_maxi_bvalid             ),  // in
      .M_AXI_GP0_rlast    (  gp0_maxi_rlast              ),  // in
      .M_AXI_GP0_rvalid   (  gp0_maxi_rvalid             ),  // in
      .M_AXI_GP0_wready   (  gp0_maxi_wready             ),  // in
      .M_AXI_GP0_bid      (  gp0_maxi_bid                ),  // in 12
      .M_AXI_GP0_rid      (  gp0_maxi_rid                ),  // in 12
      .M_AXI_GP0_bresp    (  gp0_maxi_bresp              ),  // in 2
      .M_AXI_GP0_rresp    (  gp0_maxi_rresp              ),  // in 2
      .M_AXI_GP0_rdata    (  gp0_maxi_rdata              ),  // in 32

      // SPI0
      .SPI0_SS_I          (  spi_ss_i                    ),  // in
      .SPI0_SS_O          (  spi_ss_o                    ),  // out
      .SPI0_SS1_O         (  spi_ss1_o                   ),  // out
      .SPI0_SS2_O         (  spi_ss2_o                   ),  // out
      .SPI0_SCLK_I        (  spi_sclk_i                  ),  // in
      .SPI0_SCLK_O        (  spi_sclk_o                  ),  // out
      .SPI0_MOSI_I        (  spi_mosi_i                  ),  // in
      .SPI0_MOSI_O        (  spi_mosi_o                  ),  // out
      .SPI0_MISO_I        (  spi_miso_i                  ),  // in
      .SPI0_MISO_O        (  spi_miso_o                  ),  // out
      .SPI0_SS_T          (                              ),  // out
      .SPI0_SCLK_T        (                              ),  // out
      .SPI0_MOSI_T        (                              ),  // out
      .SPI0_MISO_T        (                              )   // out
      );

   assign gp0_maxi_arstn = fclk_rstn[0] ;

endmodule

