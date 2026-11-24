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
OutputDir=.
OutputBaseFilename=LaPautaVerb-Setup-1.0.0
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Files]
Source: "La Pauta Verb.vst3\*"; DestDir: "{commoncf64}\VST3\La Pauta Verb.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
