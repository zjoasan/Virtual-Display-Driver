@echo off
echo ==================================================
echo VDD experimental Installer - Multi-Card Branch
echo ==================================================
echo.
echo Installing test certificate...
certutil -addstore root "%~dp0MttVDD.cer" >nul 2>&1
certutil -addstore -f TrustedPublisher "%~dp0MttVDD.cer"
if %errorlevel% neq 0 (
    echo ERROR: Failed to install certificate. Are you running as Administrator?
    pause
    exit /b 1
) else (
    echo [OK] Test certificate installed (Root store)
)

echo.
echo Installing driver...
pnputil /add-driver "%~dp0MttVDD.inf" /install >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Driver installation failed. Reboot might be needed.
	echo -----
	echo Try manual installation in DeviceManager, Action, Legacy...
    pause
    exit /b 1
) else (
    echo [OK] Driver installed successfully!
)
set "FOLDER=C:\VirtualDisplayDriver"
set "NEWFILE=vdd_settings.xml"
set "OLDFILE=old_vdd_settings.xml"

if exist "%FOLDER%" (
    if exist "%FOLDER%\%NEWFILE%" (
        ren "%FOLDER%\%NEWFILE%" "%FOLDER%\%OLDFILE%"
    ) else (
        copy %NEWFILE%" %FOLDER%\%NEWFILE%
    )
) else (
    md %FOLDER%
    copy %NEWFILE%" %FOLDER%\%NEWFILE%
)
copy "uninstall.bat" %FOLDER%\
copy "MttVDD.inf" %FOLDER%\
copy "MttVDD.dll" %FOLDER%\
copy "MttVDD.cer" %FOLDER%\
copy "mttvdd.cat" %FOLDER%\
copy "install_experimental_VDD.bat" %FOLDER%\

echo.
echo ==================================================
echo Feedback is really appricated.
echo ==================================================
pause