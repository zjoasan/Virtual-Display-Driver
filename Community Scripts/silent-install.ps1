[CmdletBinding()]
param(
    # SHA256 hash of the DevCon binary to install
    # Possible values can be found at:
    # https://github.com/Drawbackz/DevCon-Installer/blob/master/devcon_sources.json
    # Look for the "sha256" field in the JSON for valid hash values
    [Parameter(Mandatory=$true)]
    [string]$DevconHash,
    
    # Latest stable version of VDD
    [Parameter(Mandatory=$false)]
    [string]$DriverVersion = "latest"
)

# Create temp directory
$tempDir = Join-Path $env:TEMP "VDDInstall"
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

# Download and run DevCon Installer
Write-Host "Installing DevCon..." -ForegroundColor Cyan
$devconPath = Join-Path $tempDir "Devcon.Installer.exe"
Invoke-WebRequest -Uri "https://github.com/Drawbackz/DevCon-Installer/releases/download/1.4-rc/Devcon.Installer.exe" -OutFile $devconPath
Start-Process -FilePath $devconPath -ArgumentList "install -hash $DevconHash -update -dir `"$tempDir`"" -Wait -NoNewWindow

# Define path to devcon executable
$devconExe = Join-Path $tempDir "devcon.exe"

# Handle driver download
if ($DriverVersion -eq "latest") {
    Write-Host "Determining latest driver version..." -ForegroundColor Cyan
    $apiUrl = "https://api.github.com/repos/VirtualDrivers/Virtual-Display-Driver/releases/latest"
    $headers = @{
        "Accept" = "application/vnd.github.v3+json"
        "User-Agent" = "PowerShell-VDDInstaller"
    }
    
    try {
        $releaseInfo = Invoke-RestMethod -Uri $apiUrl -Headers $headers
        $latestVersion = $releaseInfo.tag_name
        # Look for the x64 zip asset
        $asset = $releaseInfo.assets | Where-Object { $_.name -match "x64\.zip$" } | Select-Object -First 1
        
        if ($asset) {
            $downloadUrl = $asset.browser_download_url
            Write-Host "Found latest version: $latestVersion" -ForegroundColor Cyan
        } else {
            throw "Could not find x64 driver package in latest release"
        }
    } catch {
        Write-Host "Error fetching latest release information: $_" -ForegroundColor Red
        exit 1
    }
} else {
    # Use specified version
    $downloadUrl = "https://github.com/VirtualDrivers/Virtual-Display-Driver/releases/download/$DriverVersion/Signed-Driver-v$DriverVersion-x64.zip"
}

# Download and extract driver package
Write-Host "Downloading driver from: $downloadUrl" -ForegroundColor Cyan
$driverZipPath = Join-Path $tempDir "driver.zip"
Invoke-WebRequest -Uri $downloadUrl -OutFile $driverZipPath
Expand-Archive -Path $driverZipPath -DestinationPath $tempDir -Force

# Install the driver
Write-Host "Installing virtual display driver..." -ForegroundColor Cyan
Push-Location $tempDir
& $devconExe install .\MttVDD.inf "Root\MttVDD"
Pop-Location

Write-Host "Driver installation completed." -ForegroundColor Green