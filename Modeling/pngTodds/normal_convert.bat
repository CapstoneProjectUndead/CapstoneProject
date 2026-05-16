@echo off
rem 노말맵 전용 (순수 데이터 형태 유지)
for %%f in (*.png) do (
    rem -srgbi: 입력 이미지의 sRGB 프로필 무시하고 선형 데이터로 취급
    texconv.exe -f BC7_UNORM -srgbi -m 0 -y "%%f"
)
pause