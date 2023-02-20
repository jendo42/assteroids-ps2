@echo off
for %%i in ("%PS2EMU%") do ( set image=%%~ni%%~xi )
taskkill /IM %image% /F > nul
start %PS2EMU% --console --elf %1
