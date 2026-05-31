@echo off
chcp 65001 >nul

title DirectX 12 Skybox CubeMap Assembler
cls

echo =======================================================
echo         DirectX12 스카이박스 큐브맵 제작 스크립트
echo =======================================================
echo.

:: 필수 도구 존재 여부 체크
if not exist "texassemble.exe" (
    echo [오류] 폴더 내에 'texassemble.exe' 파일이 존재하지 않습니다.
    echo DirectXTex 패키지에서 해당 파일을 복사해 넣어주세요.
    echo.
    pause
    exit /b
)

if not exist "texconv.exe" (
    echo [오류] 폴더 내에 'texconv.exe' 파일이 존재하지 않습니다.
    echo.
    pause
    exit /b
)

:: 6개의 필수 이미지 파일 존재 여부 체크
set "missing_file="
for %%f in (right.jpg left.jpg top.jpg bottom.jpg front.jpg back.jpg) do (
    if not exist "%%f" (
        echo [오류] 필수 파일 '%%f' 이^(가^) 존재하지 않습니다.
        set "missing_file=1"
    )
)

if defined missing_file (
    echo.
    pause
    exit /b
)

echo [정보] 6개의 이미지를 정상 수집했습니다.
echo [단계 1] 큐브맵 조립을 시작합니다 (texassemble)...
echo -------------------------------------------------------

:: 임시 파일 없이 곧바로 'skybox_cubemap.dds' 구조 생성
texassemble.exe cube -o skybox_cubemap.dds right.jpg left.jpg top.jpg bottom.jpg front.jpg back.jpg

if %ERRORLEVEL% NEQ 0 (
    echo [실패] 이미지 조립 단계를 실패했습니다.
    pause
    exit /b
)

echo -------------------------------------------------------
echo [단계 2] DX12 렌더링 최적화를 위한 텍스처 압축을 시작합니다 (texconv)...
echo -------------------------------------------------------

:: 생성된 'skybox_cubemap.dds'를 BC3 포맷으로 곧바로 압축 및 덮어쓰기(-y)
texconv.exe -f BC3_UNORM -y -o . skybox_cubemap.dds

echo -------------------------------------------------------
if %ERRORLEVEL% EQU 0 (
    echo [성공] 완벽한 큐브맵 구조의 'skybox_cubemap.dds' 파일 생성이 완료되었습니다!
    echo [안내] 이 파일을 프로젝트의 텍스처 폴더(예: ../Modeling/tex/)로 이동하여 사용하세요.
) else (
    echo [실패] 최종 압축 변환 과정 중 오류가 발생했습니다.
)
echo.
pause