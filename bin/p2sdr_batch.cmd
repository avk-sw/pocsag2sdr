@echo off
:start
for /F "tokens=*" %%A in (%1) do pocsag2sdr -w \\.\com1 -t 500 1 0 "%%A" & timeout 1
goto start
