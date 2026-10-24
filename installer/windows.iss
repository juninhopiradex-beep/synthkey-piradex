; Instalador Windows do SYNTHKEY by Piradex (Inno Setup 6)
; Compilado no GitHub Actions: ISCC /DVersao=1.3.0 installer\windows.iss

#ifndef Versao
  #define Versao "1.3.0"
#endif

[Setup]
AppId={{B7D2A4E9-3C51-4F08-A6E2-5D9C1F7B8A34}
AppName=SYNTHKEY by Piradex
AppVersion={#Versao}
AppVerName=SYNTHKEY by Piradex {#Versao}
AppPublisher=Piradex
DefaultDirName={commonpf64}\Piradex\SYNTHKEY
DisableDirPage=yes
DisableProgramGroupPage=yes
UninstallDisplayName=SYNTHKEY by Piradex
OutputDir=..\out-win
OutputBaseFilename=SYNTHKEY-by-Piradex-{#Versao}-Windows-Setup
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
MinVersion=10.0
Compression=lzma2/normal
SolidCompression=no
WizardStyle=modern
InfoBeforeFile=LEIA-ME-Windows.txt

[Languages]
Name: "pt"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "ptbr"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Completa (plugin + aplicação + packs)"
Name: "plugin"; Description: "Só o plugin e a aplicação"
Name: "custom"; Description: "Personalizada"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Plugin VST3"; Types: full plugin custom; Flags: fixed
Name: "app"; Description: "Aplicação independente (Standalone)"; Types: full plugin custom
Name: "packs"; Description: "Packs de samples (Trap, R&B, Afrobeats, Amapiano, Drill, Kizomba, Kuduro)"; Types: full custom

[Files]
Source: "..\build\SynthKey_artefacts\Release\VST3\SYNTHKEY by Piradex.vst3\*"; \
  DestDir: "{commoncf64}\VST3\SYNTHKEY by Piradex.vst3"; Components: vst3; \
  Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\SynthKey_artefacts\Release\Standalone\SYNTHKEY by Piradex.exe"; \
  DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "..\dist-packs\*.bfpack"; \
  DestDir: "{commonappdata}\Piradex\SYNTHKEY\Packs"; Components: packs; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\SYNTHKEY by Piradex"; Filename: "{app}\SYNTHKEY by Piradex.exe"; Components: app

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\SYNTHKEY by Piradex.vst3"
