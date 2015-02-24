@echo off
set mypath=%~dp0
set bitcrystalpath=%appdata%\BitCrystal_V20
set bitcrystalconf=%bitcrystalpath%\bitcrystal.conf
pause
echo %bitcrystalpath%
echo %bitcrystalconf%
echo %mypath%

pause
IF NOT EXIST "%bitcrystalpath%" (
		mkdir "%bitcrystalpath%"
)
pause
IF NOT EXIST "%bitcrystalconf%" (
	IF NOT EXIST "%mypath%\bitcrystal.conf" (
		wget http://bitcrystaldownload.demon-craft.de/bitcrystal_conf_update/bitcrystal.conf
		copy /b "%mypath%\bitcrystal.conf" "%bitcrystalconf%"
		pause
	) ELSE (
		pause
		copy /b "%mypath%\bitcrystal.conf" "%bitcrystalconf%"
	)
)
del /f /q /s "%mypath%bitcrystal_conf_update.txt" 1> nul 2> nul
wget http://bitcrystaldownload.demon-craft.de/bitcrystal_conf_update/bitcrystal_conf_update.txt
copy /b "%bitcrystalconf%"+"%mypath%bitcrystal_conf_update.txt" "%bitcrystalconf%"