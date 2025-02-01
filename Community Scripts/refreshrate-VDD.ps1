param(
    [Parameter(Mandatory, Position = 0)]
    [ValidateSet(
        30, 
        60,
        75,
        90,
        120,
        144,
        165,
        240,
        480,
        500)]
    [int]$refreshrate
)

# Self-elevate the script if required
if (-Not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] 'Administrator')) {
    if ([int](Get-CimInstance -Class Win32_OperatingSystem | Select-Object -ExpandProperty BuildNumber) -ge 6000) {
        $CommandLine = "-File `"" + $MyInvocation.MyCommand.Path + "`" "
        $MyInvocation.BoundParameters.GetEnumerator() | ForEach-Object { $CommandLine += "-$($_.Key):$($_.Value) " }
        Start-Process -WorkingDirectory $PSScriptRoot -FilePath PowerShell.exe -Verb Runas -ArgumentList $CommandLine  
        exit
    }
}

# install dependencies if required
. "$PSScriptRoot\set-dependencies.ps1"

# Getting the correct display
$disp = Get-DisplayInfo | Where-Object { $_.DisplayName -eq "VDD by MTT" }

# Setting the refresh rate on the correct display.
Set-DisplayRefreshRate -DisplayId $disp.DisplayId -RefreshRate $rate