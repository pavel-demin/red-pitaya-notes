library extio_red_pitaya;

{$mode objfpc}{$H+}

uses
  classes,
  controls,
  forms,
  interfaces,
  registry,
  spin,
  ssockets,
  stdctrls,
  sysutils;

type
  TCallback = procedure(size, code: Int32; offset: Single; data: Pointer); cdecl;

  TDataThread = class(TThread)
  public
    constructor Create;
  protected
    procedure Execute; override;
  end;

  TMode = record
    name: String;
    rates: array of Int32;
  end;

  TState = class
    rgst: TRegistry;
    ctrl: TInetSocket;
    data: TInetSocket;
    thrd: TDataThread;
    call: TCallback;
    form: TForm;
    addr: TEdit;
    mode: TComboBox;
    rate: TComboBox;
    corr: TSpinEdit;
    freq: Int32;
    procedure ExitComboBox(Sender: TObject);
    procedure ChangeMode(Sender: TObject);
    procedure ChangeRate(Sender: TObject);
    procedure ChangeCorr(Sender: TObject);
  end;

const

  path: String = '\Software\ExtIO_Red_Pitaya';

  fmax: Int32 = 260000000;

  modes: array of TMode = (
    (
      name: '125 MSPS';
      rates: (20, 50, 100, 250, 500, 1250);
    ),
    (
      name: '122.88 MSPS';
      rates: (24, 48, 96, 192, 384, 768, 1536);
    )
  );

var
  s: TState;
  b: array [0..65535] of Byte;

function SetHWLO(freq: Int32): Int32; stdcall; forward;

constructor TDataThread.Create;
begin
  FreeOnTerminate := True;
  inherited Create(false);
end;

procedure TDataThread.Execute;
var
  size, limit, offset: Int32;
begin
  limit := 65536;
  offset := 0;
  repeat
    size := limit - offset;
    size := s.data.Read(b[offset], size);
    offset := offset + size;
    if offset = limit then
    begin
      offset := 0;
      s.call(8192, 0, 0.0, @b);
    end;
  until Terminated;
end;

procedure SendCommand(code, value: UInt32);
var
  buffer: UInt32;
begin
  buffer := (code shl 28) or (value and $fffffff);
  if s.ctrl <> nil then s.ctrl.Write(buffer, 4);
end;

procedure UpdateRate(rate: Int32);
begin
  SendCommand(1, rate);
  if s.call <> nil then s.call(-1, 100, 0.0, nil);
end;

procedure UpdateMode(mode, rate: Int32);
var
  r, rmax: Int32;
begin
  rmax := High(modes[mode].rates);
  s.rate.Items.Clear;
  for r in modes[mode].rates do s.rate.Items.Add(IntToStr(r) + ' kSPS');
  if rate > rmax then rate := rmax;
  s.rate.ItemIndex := rate;
  UpdateRate(rate);
end;

procedure TState.ExitComboBox(Sender: TObject);
begin
  (Sender as TComboBox).SelLength := 0;
end;

procedure TState.ChangeMode(Sender: TObject);
begin
  UpdateMode(s.mode.ItemIndex, s.rate.ItemIndex);
end;

procedure TState.ChangeRate(Sender: TObject);
begin
  UpdateRate(s.rate.ItemIndex);
end;

procedure TState.ChangeCorr(Sender: TObject);
begin
  SetHWLO(s.freq);
end;

procedure SetupControl(c: TWinControl; p: TForm; e: TNotifyEvent; y: Int32);
begin
  with c do
  begin
    Parent := p;
    OnExit := e;
    Width := 112;
    Left := 136;
    Top := y;
  end;
end;

procedure CreateLabel(f: TForm; c: String; s: TControl);
var
  l: TLabel;
begin
  l := TLabel.create(f);
  with l do
  begin
    Parent := f;
    Caption := c;
    Left := 8;
    AnchorVerticalCenterTo(s);
  end;
end;

procedure Init;
var
  addr: String;
  mode, rate, corr: Int32;
  rmax, mmax: Int32;
  m: TMode;
begin
  addr := '192.168.1.100';
  mode := 0;
  rate := 2;
  corr := 0;
  s := TState.Create;
  s.rgst := TRegistry.Create;
  with s.rgst do
  begin
    OpenKey(path, True);
    if ValueExists('Addr') then addr := ReadString('Addr') else WriteString('Addr', addr);
    if ValueExists('Mode') then mode := ReadInteger('Mode') else WriteInteger('Mode', mode);
    if ValueExists('Rate') then rate := ReadInteger('Rate') else WriteInteger('Rate', rate);
    if ValueExists('Corr') then corr := ReadInteger('Corr') else WriteInteger('Corr', corr);
  end;
  mmax := High(modes);
  if mode < 0 then mode := 0;
  if mode > mmax then mode := mmax;
  rmax := High(modes[mode].rates);
  if rate < 0 then rate := 0;
  if rate > rmax then rate := rmax;
  if corr < -100 then corr := -100;
  if corr > 100 then corr := 100;

  Application.Scaled := True;
  Application.Initialize;

  s.form := TForm.Create(nil);
  with s.form do
  begin
    BorderStyle := bsSingle;
    Caption := 'Settings';
    PixelsPerInch := 96;
    Height := 132;
    Width := 256;
  end;

  s.addr := TEdit.Create(s.form);
  SetupControl(s.addr, s.form, nil, 8);
  s.addr.Text := addr;

  CreateLabel(s.form, 'IP address', s.addr);

  s.mode := TComboBox.Create(s.form);
  SetupControl(s.mode, s.form, @s.ExitComboBox, 40);
  with s.mode do
  begin
    ReadOnly := True;
    OnChange := @s.ChangeMode;
    for m in modes do Items.Add(m.name);
    ItemIndex := mode;
  end;

  CreateLabel(s.form, 'ADC sample rate', s.mode);

  s.rate := TComboBox.Create(s.form);
  SetupControl(s.rate, s.form, @s.ExitComboBox, 72);
  s.rate.ReadOnly := True;
  s.rate.OnChange := @s.ChangeRate;
  UpdateMode(mode, rate);

  CreateLabel(s.form, 'SDR sample rate', s.rate);

  s.corr := TSpinEdit.Create(s.form);
  SetupControl(s.corr, s.form, nil, 104);
  with s.corr do
  begin
    Alignment := taRightJustify;
    MinValue := -100;
    MaxValue := 100;
    OnChange := @s.ChangeCorr;
    Value := corr;
  end;

  CreateLabel(s.form, 'Freq. corr. (ppm)', s.corr);
end;

procedure Free;
begin
  s.rgst.WriteString('Addr', s.addr.Text);
  s.rgst.WriteInteger('Mode', s.mode.ItemIndex);
  s.rgst.WriteInteger('Rate', s.rate.ItemIndex);
  s.rgst.WriteInteger('Corr', s.corr.Value);
  s.form.Free;
  s.rgst.Free;
  s.Free;
end;

function InitHW(name, model: PChar; var format: Int32): Boolean; stdcall;
begin
  format := 7;
  strpcopy(name, 'Red Pitaya');
  strpcopy(model, '');
  Result := True;
end;

function OpenHW: Boolean; stdcall;
begin
  Result := True;
end;

function StartHW(freq: Int32): Int32; stdcall;
var
  addr: String;
  command: UInt32;
begin
  addr := s.addr.Text;
  try
    s.ctrl := TInetSocket.Create(addr, 1001, 1000);
    s.data := TInetSocket.Create(addr, 1001, 1000);
  except
    FreeAndNil(s.ctrl);
    FreeAndNil(s.data);
    Result := -1;
    Exit;
  end;
  command := 0;
  s.ctrl.Write(command, 4);
  command := 1;
  s.data.Write(command, 4);
  SetHWLO(freq);
  SendCommand(1, s.rate.ItemIndex);
  s.thrd := TDataThread.Create;
  Result := 8192;
end;

procedure StopHW; stdcall;
begin
  s.thrd.Terminate;
  Sleep(200);
  FreeAndNil(s.ctrl);
  FreeAndNil(s.data);
end;

procedure CloseHW; stdcall;
begin

end;

procedure SetCallback(call: TCallback); stdcall;
begin
  s.call := call;
end;

function SetHWLO(freq: Int32): Int32; stdcall;
begin
  if freq > fmax then
  begin
    s.freq := fmax;
    Result := fmax;
  end
  else
  begin
    s.freq := freq;
    Result := 0;
  end;
  SendCommand(0, Round(s.freq * (1.0 + s.corr.Value * 1.0e-6)));
  if (s.freq <> freq) and (s.call <> nil) then s.call(-1, 101, 0.0, nil);
end;

function GetHWLO: Int32; stdcall;
begin
  Result := s.freq;
end;

function GetHWSR: Int32; stdcall;
begin
  Result := modes[s.mode.ItemIndex].rates[s.rate.ItemIndex] * 1000;
end;

function GetStatus: Int32; stdcall;
begin
  Result := 0;
end;

procedure ShowGUI; stdcall;
begin
  if not s.form.Visible then s.form.Show;
end;

procedure HideGUI; stdcall;
begin
  if s.form.Visible then s.form.Hide;
end;

exports
  InitHW,
  OpenHW,
  StartHW,
  StopHW,
  CloseHW,
  SetCallback,
  SetHWLO,
  GetHWLO,
  GetHWSR,
  GetStatus,
  ShowGUI,
  HideGUI;

begin
  Init;
  ExitProc := @Free;
end.
