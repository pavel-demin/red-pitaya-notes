
`timescale 1 ns / 1 ps

module axis_gate_controller
(
  input  wire         aclk,
  input  wire         aresetn,

  input  wire [127:0] s_axis_tx_evts_tdata,
  input  wire         s_axis_tx_evts_tvalid,
  output wire         s_axis_tx_evts_tready,

  input  wire [63:0]  s_axis_rx_evts_tdata,
  input  wire         s_axis_rx_evts_tvalid,
  output wire         s_axis_rx_evts_tready,

  input  wire [127:0] s_axis_tdata,
  input  wire         s_axis_tvalid,
  output wire         s_axis_tready,

  output wire [127:0] m_axis_tdata,
  output wire         m_axis_tvalid,

  output wire [29:0]  tx_phase,
  output wire [29:0]  rx_phase,

  output wire [15:0]  level,

  output wire         sync,
  output wire         gate,
  output wire         enbl
);

  reg [39:0] tx_cntr_reg;
  reg [83:0] tx_data_reg;
  reg tx_sync_reg, tx_gate_reg;

  wire [1:0] tx_data_wire;
  wire tx_enbl_wire;

  assign tx_data_wire = tx_enbl_wire ? tx_data_reg[1:0] : s_axis_tx_evts_tdata[41:40];
  assign tx_enbl_wire = |tx_cntr_reg;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      tx_cntr_reg <= 40'd0;
      tx_data_reg <= 84'd0;
      tx_sync_reg <= 1'b0;
      tx_gate_reg <= 1'b0;
    end
    else
    begin
      if(tx_enbl_wire)
      begin
        tx_cntr_reg <= tx_cntr_reg - 1'b1;
      end
      else if(s_axis_tx_evts_tvalid)
      begin
        tx_cntr_reg <= s_axis_tx_evts_tdata[39:0];
        tx_data_reg <= s_axis_tx_evts_tdata[123:40];
      end
      tx_sync_reg <= tx_data_wire[0] & (tx_enbl_wire | s_axis_tx_evts_tvalid);
      tx_gate_reg <= tx_data_wire[1] & (tx_enbl_wire | s_axis_tx_evts_tvalid);
    end
  end

  reg [39:0] rx_cntr_reg;
  reg rx_data_reg;

  reg [127:0] rx_tdata_reg;
  reg rx_enbl_reg, rx_tvalid_reg;

  wire rx_data_wire;
  wire rx_enbl_wire;

  assign rx_data_wire = rx_enbl_wire ? rx_data_reg : s_axis_rx_evts_tdata[40];
  assign rx_enbl_wire = |rx_cntr_reg;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      rx_cntr_reg <= 40'd0;
      rx_data_reg <= 1'b0;
      rx_enbl_reg <= 1'b0;
      rx_tvalid_reg <= 1'b0;
    end
    else
    begin
      if(s_axis_tvalid)
      begin
        if(rx_enbl_wire)
        begin
          rx_cntr_reg <= rx_cntr_reg - 1'b1;
        end
        else if(s_axis_rx_evts_tvalid)
        begin
          rx_cntr_reg <= s_axis_rx_evts_tdata[39:0];
          rx_data_reg <= s_axis_rx_evts_tdata[40];
        end
      end
      rx_enbl_reg <= rx_enbl_wire | s_axis_rx_evts_tvalid;
      rx_tvalid_reg <= s_axis_tvalid & rx_data_wire & (rx_enbl_wire | s_axis_rx_evts_tvalid);
    end
    rx_tdata_reg <= s_axis_tdata;
  end

  assign s_axis_tx_evts_tready = ~tx_enbl_wire & aresetn;

  assign s_axis_rx_evts_tready = ~rx_enbl_wire & aresetn & s_axis_tvalid;

  assign s_axis_tready = 1'b1;

  assign m_axis_tdata = rx_tdata_reg;
  assign m_axis_tvalid = rx_tvalid_reg;

  assign tx_phase = tx_data_reg[53:24];
  assign rx_phase = tx_data_reg[83:54];

  assign level = tx_data_reg[19:4];

  assign sync = tx_sync_reg;
  assign gate = tx_gate_reg;
  assign enbl = rx_enbl_reg;

endmodule
