; Instalador de La Pauta Verb (Inno Setup 6)
[Setup]
AppId={{B7F3C2A1-5D4E-4A8B-9C61-2E7D1F0A9B34}
AppName=La Pauta Verb
AppVersion=1.0.0
AppPublisher=La Pauta
DefaultDirName={autopf}\La Pauta\La Pauta Verb
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\installer_output
OutputBaseFilename=LaPautaVerb-Setup-1.0.0
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=La Pauta Verb (VST3)

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Files]
Source: "..\build\LaPautaVerb_artefacts\Release\VST3\La Pauta Verb.vst3\*"; \
    DestDir: "{commoncf64}\VST3\La Pauta Verb.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

[Messages]
spanish.FinishedLabel=La Pauta Verb se instalo en la carpeta VST3. Reescanea los plugins en tu DAW.
