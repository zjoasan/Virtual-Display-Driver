# Self-elevate the script if required
if (-Not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] 'Administrator')) {
    if ([int](Get-CimInstance -Class Win32_OperatingSystem | Select-Object -ExpandProperty BuildNumber) -ge 6000) {
        $CommandLine = "-File `"" + $MyInvocation.MyCommand.Path + "`" " + $MyInvocation.UnboundArguments
        Start-Process -FilePath PowerShell.exe -Verb Runas -ArgumentList $CommandLine
        Exit
    }
}

# Install dependencies if required
. "$PSScriptRoot\set-dependencies.ps1"

# Getting the correct display
$disp = Get-DisplayInfo | Where-Object { $_.DisplayName -eq "VDD by MTT" }

$bpcc = Get-DisplayColorInfo -DisplayId $disp.DisplayId | Select-Object BitsPerColorChannel | Select-Object -ExpandProperty BitsPerColorChannel
 
if ( $bpcc -is [int] ) {
    if ( $bpcc -gt 8 ) {
        Disable-DisplayAdvancedColor -DisplayId $disp
	    break
	}
	if ( $bpcc -lt 10 ) {
        Enable-DisplayAdvancedColor -DisplayId $disp
	    break
	}
}
else { write-error "Something went wrong in identifying the Colormode of the display." ; break }