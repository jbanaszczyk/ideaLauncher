param(
    [string]$LauncherYaml = ".\data\launcher.yaml"
)

if (-not (Test-Path $LauncherYaml)) {
    Write-Error "File $LauncherYaml not found"
    exit 1
}

# Read YAML file to determine exe_name
$yaml = Get-Content $LauncherYaml -Raw -Encoding UTF8

# Extract exe_name from YAML using regex; default to idea64.exe
$exeName               = if ($yaml -match "(?m)^\s*exe_name\s*:\s*(.+)$") { $matches[1].Trim() } else { "idea64.exe" }
# original vmoptions filename is always derived from exe_name
$originalVmoptionsName = "$exeName.vmoptions"
# Properties original filename is stable: idea.properties
$originalPropertiesName= "idea.properties"

# Function: compute SHA-256 for a file
function Get-FileSha256($path) {
    if (-not (Test-Path $path)) { return $null }
    (Get-FileHash $path -Algorithm SHA256).Hash.ToLower()
}

# Build paths to original files inside .\app\bin\
$tplVmPath  = Join-Path ".\app\bin" $originalVmoptionsName
$tplPropPath= Join-Path ".\app\bin" $originalPropertiesName

$shaVm  = Get-FileSha256 $tplVmPath
$shaProp= Get-FileSha256 $tplPropPath

if (-not $shaVm)   { Write-Error "Missing file $tplVmPath"; exit 2 }
if (-not $shaProp) { Write-Error "Missing file $tplPropPath"; exit 2 }

# Print only the two YAML-ready lines for copy/paste
"original_vmoptions_sha: $shaVm"
"original_properties_sha: $shaProp"
