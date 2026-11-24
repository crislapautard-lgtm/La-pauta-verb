#define MyAppName "La Pauta Verb"
#define MyAppVersion "1.0.0"
#define MyPublisher "La Pauta"
#define MyVst3Path "..\\build\\LaPautaVerb_artefacts\\Release\\VST3\\La Pauta Verb.vst3"

[Setup]
AppId={{C9A7B3F1-5B26-4B17-9A70-8D9B9C9A1F30}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyPublisher}
DefaultDirName={commoncf}\VST3\La Pauta Verb.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64
OutputDir=..\dist
OutputBaseFilename=La-Pauta-Verb-Setup-1.0.0-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
Uninstallable=yes

[Files]
Source: "{#MyVst3Path}\\*"; DestDir: "{commoncf}\VST3\La Pauta Verb.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\La Pauta Verb.vst3"

[Messages]
WelcomeLabel2=Este asistente instalará [name] en la carpeta VST3 compartida de Windows.%n%nDespués de instalarlo, abre tu DAW y vuelve a escanear los plugins VST3.
FinishedLabel=La Pauta Verb se instaló correctamente.%n%nAbre tu DAW y vuelve a escanear los plugins VST3 para comenzar a usarlo.
