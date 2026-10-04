unit Unit2;

{ Universal Recoil v2.0 - reconstructed from UR_3_1.exe (Delphi 12 / VCL).

  Behaviour recovered from the compiled event handlers:
    F1 / F2     enable / disable Timer1 (polled every 1 ms by Timer2)
    Timer1      while the selected "Aim Key" is held, nudge the mouse by
                (Right, Down), Sleep(Delay), then by (Left, Up), Sleep(Delay)
    Save / Load INI file, section [Recoil]: Right, Down, Left, Up, Delay }

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants,
  System.Classes, System.IniFiles, Vcl.Graphics, Vcl.Controls, Vcl.Forms,
  Vcl.Dialogs, Vcl.ComCtrls, Vcl.StdCtrls, Vcl.ExtCtrls, Vcl.Samples.Spin;

type
  TMainForm = class(TForm)
    StatusBar1: TStatusBar;
    TabControl1: TTabControl;
    TrackBar1: TTrackBar;   // Up    (-20..0)
    TrackBar2: TTrackBar;   // Down  (0..20)
    TrackBar3: TTrackBar;   // Right (0..20)
    TrackBar4: TTrackBar;   // Left  (-20..0)
    Label1: TLabel;         // value of TrackBar1 (Up)
    Label2: TLabel;         // value of TrackBar2 (Down)
    Label3: TLabel;         // value of TrackBar3 (Right)
    Label4: TLabel;         // value of TrackBar4 (Left)
    Timer1: TTimer;         // the recoil loop (disabled until F1)
    SpinEdit1: TSpinEdit;   // Delay in ms (0..50)
    Label5: TLabel;
    Label6: TLabel;
    Label7: TLabel;
    Label8: TLabel;
    Timer2: TTimer;         // hotkey poller
    Label9: TLabel;
    Label10: TLabel;        // value of SpinEdit1 (Delay)
    SaveDialog1: TSaveDialog;
    Button1: TButton;       // Save
    OpenDialog1: TOpenDialog;
    Button2: TButton;       // Load
    Panel1: TPanel;         // crosshair preview
    Image1: TImage;
    Image2: TImage;
    Image3: TImage;
    Image4: TImage;
    Image5: TImage;
    Label11: TLabel;
    Label12: TLabel;
    ComboBox1: TComboBox;   // Aim Key
    Label13: TLabel;
    CheckBox1: TCheckBox;   // Advanced Mode
    procedure Timer1Timer(Sender: TObject);
    procedure TrackBar1Change(Sender: TObject);
    procedure TrackBar2Change(Sender: TObject);
    procedure TrackBar3Change(Sender: TObject);
    procedure TrackBar4Change(Sender: TObject);
    procedure Timer2Timer(Sender: TObject);
    procedure SpinEdit1Change(Sender: TObject);
    procedure Button1Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure CheckBox1Click(Sender: TObject);
  end;

var
  MainForm: TMainForm;

implementation

{$R *.dfm}

procedure TMainForm.CheckBox1Click(Sender: TObject);
begin
  if CheckBox1.Checked then
  begin
    TrackBar2.Max := 250;
    TrackBar3.Max := 250;
    TrackBar1.Min := -250;
    TrackBar4.Min := -250;
    SpinEdit1.MaxValue := 5000;
  end
  else
  begin
    TrackBar2.Max := 20;
    TrackBar3.Max := 20;
    TrackBar1.Min := -20;
    TrackBar4.Min := -20;
    SpinEdit1.MaxValue := 50;
  end;
end;

procedure TMainForm.SpinEdit1Change(Sender: TObject);
begin
  Label10.Caption := IntToStr(SpinEdit1.Value);
end;

// The four TrackBar handlers update the value label and, for values inside
// +/-10, move the dot on the preview panel. (Compiled as `case` jump tables;
// written here as the equivalent arithmetic.)

procedure TMainForm.TrackBar1Change(Sender: TObject);
begin
  Label1.Caption := IntToStr(TrackBar1.Position);
  if (TrackBar1.Position >= -10) and (TrackBar1.Position <= 0) then
    Image5.Top := 45 + 2 * TrackBar1.Position;
end;

procedure TMainForm.TrackBar2Change(Sender: TObject);
begin
  Label2.Caption := IntToStr(TrackBar2.Position);
  if (TrackBar2.Position >= 0) and (TrackBar2.Position <= 10) then
    Image4.Top := 45 + 2 * TrackBar2.Position;
end;

procedure TMainForm.TrackBar3Change(Sender: TObject);
begin
  Label3.Caption := IntToStr(TrackBar3.Position);
  if (TrackBar3.Position >= 0) and (TrackBar3.Position <= 10) then
    Image1.Left := 44 + 2 * TrackBar3.Position;
end;

procedure TMainForm.TrackBar4Change(Sender: TObject);
begin
  Label4.Caption := IntToStr(TrackBar4.Position);
  if (TrackBar4.Position >= -10) and (TrackBar4.Position <= 0) then
    Image3.Left := 44 + 2 * TrackBar4.Position;
end;

procedure TMainForm.Timer1Timer(Sender: TObject);
  procedure Nudge;
  begin
    mouse_event(MOUSEEVENTF_MOVE, StrToInt(Label3.Caption), StrToInt(Label2.Caption), 0, 0);
    Sleep(SpinEdit1.Value);
    mouse_event(MOUSEEVENTF_MOVE, StrToInt(Label4.Caption), StrToInt(Label1.Caption), 0, 0);
    Sleep(SpinEdit1.Value);
  end;
begin
  if ComboBox1.ItemIndex = 0 then
  begin
    // "Left Mouse Button"
    if GetAsyncKeyState(VK_LBUTTON) <> 0 then
      Nudge;
  end
  else if ComboBox1.ItemIndex = 1 then
  begin
    // "Right + Left Mouse Button"
    if (GetAsyncKeyState(VK_RBUTTON) and GetAsyncKeyState(VK_LBUTTON)) <> 0 then
      Nudge;
  end;
end;

procedure TMainForm.Timer2Timer(Sender: TObject);
begin
  if (GetAsyncKeyState(VK_F1) and 1) <> 0 then
  begin
    Timer1.Enabled := True;
    StatusBar1.Panels[0].Text := TimeToStr(Now);
    StatusBar1.Panels[1].Text := 'Recoil';
    StatusBar1.Panels[2].Text := '# O N #';
  end
  else if (GetAsyncKeyState(VK_F2) and 1) <> 0 then
  begin
    Timer1.Enabled := False;
    StatusBar1.Panels[0].Text := TimeToStr(Now);
    StatusBar1.Panels[1].Text := 'Recoil';
    StatusBar1.Panels[2].Text := '# O F F #';
  end;
end;

procedure TMainForm.Button1Click(Sender: TObject);
var
  Ini: TIniFile;
begin
  SaveDialog1.Filter := 'Config file|*.ini';
  SaveDialog1.DefaultExt := 'ini';
  if SaveDialog1.Execute then
  begin
    Ini := TIniFile.Create(SaveDialog1.FileName);
    try
      Ini.WriteString('Recoil', 'Right', Label3.Caption);
      Ini.WriteString('Recoil', 'Down', Label2.Caption);
      Ini.WriteString('Recoil', 'Left', Label4.Caption);
      Ini.WriteString('Recoil', 'Up', Label1.Caption);
      Ini.WriteString('Recoil', 'Delay', Label10.Caption);
      StatusBar1.Panels[0].Text := TimeToStr(Now);
      StatusBar1.Panels[1].Text := 'Recoil';
      StatusBar1.Panels[2].Text := '# S A V E D #';
    finally
      Ini.Free;
    end;
  end;
end;

procedure TMainForm.Button2Click(Sender: TObject);
var
  Ini: TIniFile;
begin
  OpenDialog1.Filter := 'Config file|*.ini';
  OpenDialog1.DefaultExt := 'ini';
  if OpenDialog1.Execute then
  begin
    Ini := TIniFile.Create(OpenDialog1.FileName);
    try
      TrackBar3.Position := StrToInt(Ini.ReadString('Recoil', 'Right', '0'));
      TrackBar2.Position := StrToInt(Ini.ReadString('Recoil', 'Down', '0'));
      TrackBar4.Position := StrToInt(Ini.ReadString('Recoil', 'Left', '0'));
      TrackBar1.Position := StrToInt(Ini.ReadString('Recoil', 'Up', '0'));
      SpinEdit1.Value := StrToInt(Ini.ReadString('Recoil', 'Delay', '0'));
      StatusBar1.Panels[0].Text := TimeToStr(Now);
      StatusBar1.Panels[1].Text := 'Recoil';
      StatusBar1.Panels[2].Text := '# L O A D E D #';
    finally
      Ini.Free;
    end;
  end;
end;

end.
