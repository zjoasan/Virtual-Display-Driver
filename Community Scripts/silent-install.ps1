# Run this script in a powershell with administrator rights (run as administrator)
[CmdletBinding()]
param(
    # SHA256 hash of the DevCon binary to install
    # Possible values can be found at:
    # https://github.com/Drawbackz/DevCon-Installer/blob/master/devcon_sources.json
    # Look for the "sha256" field in the JSON for valid hash values
    [Parameter(Mandatory=$true)]
    [string]$DevconHash,
    
    # Latest stable version of VDD driver only
    [Parameter(Mandatory=$false)]
    [string]$DriverURL = "https://github.com/VirtualDrivers/Virtual-Display-Driver/releases/download/25.7.23/VirtualDisplayDriver-x86.Driver.Only.zip"
);

# Create temp directory
$tempDir = Join-Path $env:TEMP "VDDInstall";
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null;

# Download and run DevCon Installer
Write-Host "Installing DevCon..." -ForegroundColor Cyan;
$devconPath = Join-Path $tempDir "Devcon.Installer.exe";
Invoke-WebRequest -Uri "https://github.com/Drawbackz/DevCon-Installer/releases/download/1.4-rc/Devcon.Installer.exe" -OutFile $devconPath;
Start-Process -FilePath $devconPath -ArgumentList "install -hash $DevconHash -update -dir `"$tempDir`"" -Wait -NoNewWindow;

# Define path to devcon executable
$devconExe = Join-Path $tempDir "devcon.exe";

# Check if VDD is installed. Or else, install it
$check = & $devconExe find "Root\MttVDD";
if ($check -match "1 matching device\(s\) found") {
    Write-Host "Virtual Display Driver already present. No installation." -ForegroundColor Green;
} else {
    # Download and unzip VDD
    Write-Host "Downloading VDD..." -ForegroundColor Cyan;
    $driverZipPath = Join-Path $tempDir 'driver.zip';
    Invoke-WebRequest -Uri $DriverURL -OutFile $driverZipPath;
    Expand-Archive -Path $driverZipPath -DestinationPath $tempDir -Force;

    # Extract the signPath certificates
    $catFile = Join-Path $tempDir 'VirtualDisplayDriver\mttvdd.cat';
    $signature = Get-AuthenticodeSignature -FilePath $catFile;
    $catBytes = [System.IO.File]::ReadAllBytes($catFile);
    $certificates = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2Collection;
    $certificates.Import($catBytes);

    # Create the temp directory for certificates
    $certsFolder = Join-Path $tempDir "ExportedCerts";
    New-Item -ItemType Directory -Path $certsFolder;

    # Write and store the driver certificates on local machine
    Write-Host "Installing driver certificates on local machine." -ForegroundColor Cyan;
    foreach ($cert in $certificates) {
        $certFilePath = Join-Path -Path $certsFolder -ChildPath "$($cert.Thumbprint).cer";
        $cert.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Cert) | Set-Content -Path $certFilePath -Encoding Byte;
        Import-Certificate -FilePath $certFilePath -CertStoreLocation "Cert:\LocalMachine\TrustedPublisher";
    }

    # Install VDD
    Push-Location $tempDir;
    & $devconExe install .\VirtualDisplayDriver\MttVDD.inf "Root\MttVDD";
    Pop-Location;

    Write-Host "Driver installation completed." -ForegroundColor Green;
}
Remove-Item -Path $tempDir -Recurse -Force -ErrorAction SilentlyContinue;

