; ==============================================================================
; Inno Setup Script: Atelier's Vault
; Security-first desktop password manager (C++17, Qt 6, Libsodium)
; ==============================================================================

#define MyAppName "Atelier's Vault"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Atelier"
#define MyAppURL "https://github.com/atelier/ateliervault"
#define MyAppExeName "AtelierVault.exe"
#define MyAppIconName "icon.ico"
#define MyOutputDir "..\out\dist"
#define MySourceDir "..\out\dist\AtelierVault"

[Setup]
; Unique application GUID for version tracking and clean upgrade/uninstall
AppId={{8B78DAE2-7D7E-4DF3-B85E-5374E86E2026}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

; Installation target directory ({autopf} resolves to Program Files or Local AppData programs)
DefaultDirName={autopf}\AtelierVault
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes

; Output installer packaging details
OutputDir={#MyOutputDir}
OutputBaseFilename=AtelierVault_Setup_v1.0.0
SetupIconFile=..\icon.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName}

; Compression settings (ultra lzma2)
Compression=lzma2/ultra64
SolidCompression=yes

; Architecture: 64-bit Windows requirement
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; Privileges configuration (supports both per-user and administrative install)
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog commandline

; Visual styling
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Primary application bundle and all deployed Qt runtime dependencies
Source: "{#MySourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; Embed application icon for desktop/start menu shortcuts
Source: "..\icon.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
; Start Menu shortcuts
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\{#MyAppIconName}"
Name: "{autoprograms}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"

; Optional Desktop shortcut
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\{#MyAppIconName}"; Tasks: desktopicon

[Registry]
; Clean registry registration for application and uninstallation support
Root: HKA; Subkey: "Software\{#MyAppPublisher}\{#MyAppName}"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\{#MyAppPublisher}\{#MyAppName}"; ValueType: string; ValueName: "Version"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\{#MyAppPublisher}\{#MyAppName}"; ValueType: string; ValueName: "Executable"; ValueData: "{app}\{#MyAppExeName}"; Flags: uninsdeletekey

[Run]
; Option to launch the application immediately after setup finishes
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
