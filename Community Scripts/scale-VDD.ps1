[CmdletBinding(DefaultParameterSetName = "UserDefined")]
param(
	[Parameter(Mandatory, Position = 0, ParameterSetName = "UserDefined")]
	[ValidateSet("100", "125", "150", "175", "200", "225", "250", "300", "350", "400", "450", "500")]
	[int]$scale,

	[Parameter(Mandatory, ParameterSetName = "Recommended")]
	[switch]$reset
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

# Install dependencies if required.
. "$PSScriptRoot\set-dependencies.ps1"

# Getting the correct display
$disp = Get-DisplayInfo | Where-Object { $_.DisplayName -eq "VDD by MTT" }

$maxscale = Get-DisplayScale -DisplayId $disp.DisplayId | Select-Object -ExpandProperty MaxScale
if ( $PSCmdlet.ParameterSetName -eq "UserDefined" ) {
	Write-Host "Setting scale to $scale"
	if ( $scale -le $maxscale ) {
		Set-DisplayScale -DisplayId $disp.DisplayId -Scale $scale
	}
	else { 
		Write-Error "Scale percentage $scale is higher than max scale $maxscale" 
	}
}
elseif ( $PSCmdlet.ParameterSetName -eq "Recommended" ) {
	Set-DisplayScale -DisplayId $disp.DisplayId -Recommended
}
else {
	Write-Error "Invalid arguments: 'reset' or a valid percentage value like 150 is acceptable."
}
