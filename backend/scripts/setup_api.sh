#!/bin/bash
# ============================================================================
# setup_api.sh - Configura todo para la API
# ============================================================================
# Uso: bash setup_api.sh
# ============================================================================

set -e

echo "Setup MiniC Compiler API"
echo "============================"

# 1. Instalar nlohmann/json (si no esta presente)
echo "Verificando dependencias C++..."
if ! find /usr/include -name "json.hpp" 2>/dev/null | grep -q nlohmann; then
    echo "  Instalando nlohmann/json..."
    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y nlohmann-json3-dev
    elif command -v brew &> /dev/null; then
        brew install nlohmann-json
    else
        echo "  ADVERTENCIA: nlohmann/json no encontrado. Instalalo manualmente:"
        echo "      Ubuntu/Debian: sudo apt-get install nlohmann-json3-dev"
        echo "      macOS: brew install nlohmann-json"
        echo "      o descarga desde: https://github.com/nlohmann/json"
        exit 1
    fi
else
    echo "  OK: nlohmann/json encontrado"
fi

# 2. Compilar compilador
echo ""
echo "Compilando compilador MiniC..."
if make all; then
    echo "  OK: Compilacion exitosa"
    ls -lh minicc minicc_api
else
    echo "  ERROR: Error en compilacion"
    exit 1
fi

# 3. Instalar dependencias Python
echo ""
echo "Instalando dependencias Python..."
if command -v python3 &> /dev/null; then
    python3 -m venv venv
    source venv/bin/activate
    pip install --upgrade pip
    pip install -r backend/requirements.txt
    echo "  OK: Entorno virtual creado"
else
    echo "  ERROR: Python 3 no encontrado"
    exit 1
fi

echo ""
echo "Setup completado!"
echo ""
echo "Para iniciar el backend:"
echo "  source venv/bin/activate"
echo "  python backend/src/backend.py"
echo ""
echo "O con uvicorn directamente:"
echo "  uvicorn backend.src.backend:app --reload"
echo ""
echo "API disponible en: http://localhost:8000"
echo "Documentacion: http://localhost:8000/docs"
