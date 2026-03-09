@echo off
rem texconv.exe가 있는 경로를 지정하거나 같은 폴더에 두세요.
for %%f in (*.png) do (
    texconv.exe -f BC7_UNORM -m 0 -y "%%f"
)
pause