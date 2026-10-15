# FC Project Pipeline PowerShell Shortcut Runner
param (
    [Parameter(Position=0)]
    [string]$Mode = "static",
    [string]$Asset,
    [switch]$Deep,
    [switch]$Json,
    [switch]$SkipGas,
    [switch]$Clean,
    [switch]$AutoCommit,
    [string]$Message
)

$ScriptPath = Join-Path $PSScriptRoot "Scripts\pipeline.py"
$ArgsList = @($ScriptPath, $Mode)

if ($Asset) { $ArgsList += "--asset"; $ArgsList += $Asset }
if ($Deep) { $ArgsList += "--deep" }
if ($Json) { $ArgsList += "--json" }
if ($SkipGas) { $ArgsList += "--skip-gas" }
if ($Clean) { $ArgsList += "--clean" }
if ($AutoCommit) { $ArgsList += "--auto-commit" }
if ($Message) { $ArgsList += "--message"; $ArgsList += $Message }

python @ArgsList
