@echo off
REM ============================================================================
REM setup_api.bat - Configura todo para la API en Windows
REM ============================================================================
REM Uso: setup_api.bat
REM ============================================================================

echo.
echo Compilador MiniC - Setup API
echo ============================
echo.

REM 1. Compilar compilador
echo [1/2] Compilando compilador MiniC...
mingw32-make all
if errorlevel 1 (
    echo Error en compilacion
    exit /b 1
)
echo Compilacion exitosa
echo.

REM 2. Crear entorno virtual e instalar dependencias Python
echo [2/2] Instalando dependencias Python...
python -m venv venv
call venv\Scripts\activate.bat
python -m pip install --upgrade pip
pip install -r backend\requirements.txt

echo.
echo ================== SETUP COMPLETADO ==================
echo.
echo Para iniciar el backend:
echo   1. Abre una terminal en este directorio
echo   2. Ejecuta: venv\Scripts\activate.bat
echo   3. Ejecuta: python backend\src\backend.py
echo.
echo O con uvicorn:
echo   uvicorn backend.src.backend:app --reload
echo.
echo API disponible en: http://localhost:8000
echo Documentacion: http://localhost:8000/docs
echo.
