param(
	[Parameter(Mandatory, Position = 0)]
	[ValidateSet(90, 180, 270)]
	[int]$rotate
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

# install dependencies if required.
. "$PSScriptRoot\set-dependencies.ps1"

switch ($rotate) {
	90 {
		$degree = "Rotate90"
		continue
	}
	180 {
		$degree = "Rotate180"
		continue
	}
	270 {
		$degree = "Rotate270"
		continue
	}
	default	{ Write-Error "Invalid rotation angle(valid 90,180,270)."; break }
}


# Getting the correct display
$disp = Get-DisplayInfo | Where-Object { $_.DisplayName -eq "VDD by MTT" }

# Setting the resolution on the correct display.
Set-DisplayRotation -DisplayId $disp.DisplayId -Rotation $degree
	