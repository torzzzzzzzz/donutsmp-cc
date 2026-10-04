program UR_3_1;

uses
  Vcl.Forms,
  Vcl.Themes,
  Vcl.Styles,
  Unit2 in 'Unit2.pas' {MainForm};

{$R *.res}

begin
  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  TStyleManager.TrySetStyle('Ruby Graphite');
  Application.CreateForm(TMainForm, MainForm);
  Application.Run;
end.
