#!/bin/bash
# ============================================================================
# install_json_lib.sh - Instala nlohmann/json si no esta disponible
# ============================================================================
# Descarga e instala nlohmann/json de forma manual si es necesario
# ============================================================================

set -e

NLOHMANN_VERSION="3.11.2"
INSTALL_DIR="/usr/local/include"
TEMP_DIR="/tmp/nlohmann_json"

echo "Verificando/Instalando nlohmann/json..."
echo ""

# Verificar si ya esta instalado
if find /usr/include -name "json.hpp" 2>/dev/null | grep -q nlohmann; then
    echo "OK: nlohmann/json ya esta instalado"
    exit 0
fi

if find "$INSTALL_DIR" -name "json.hpp" 2>/dev/null | grep -q nlohmann; then
    echo "OK: nlohmann/json ya esta instalado en $INSTALL_DIR"
    exit 0
fi

echo "Instalando nlohmann/json v${NLOHMANN_VERSION}..."

# Intentar instalacion nativa primero
if command -v apt-get &> /dev/null; then
    echo "  Detectado: apt-get (Ubuntu/Debian)"
    sudo apt-get update
    sudo apt-get install -y nlohmann-json3-dev
    echo "OK: Instalacion completada"
    exit 0
elif command -v brew &> /dev/null; then
    echo "  Detectado: Homebrew (macOS)"
    brew install nlohmann-json
    echo "OK: Instalacion completada"
    exit 0
elif command -v pacman &> /dev/null; then
    echo "  Detectado: pacman (Arch)"
    sudo pacman -S nlohmann-json
    echo "OK: Instalacion completada"
    exit 0
elif command -v dnf &> /dev/null; then
    echo "  Detectado: dnf (Fedora/RHEL)"
    sudo dnf install nlohmann-json-devel
    echo "OK: Instalacion completada"
    exit 0
fi

# Si no hay gestor de paquetes, descargar manualmente
echo "  No se detecto gestor de paquetes, descargando manualmente..."

mkdir -p "$TEMP_DIR"
cd "$TEMP_DIR"

# Descargar desde GitHub
echo "  Descargando desde GitHub..."
GITHUB_URL="https://github.com/nlohmann/json/releases/download/v${NLOHMANN_VERSION}/include.zip"

if command -v wget &> /dev/null; then
    wget -q "$GITHUB_URL" -O include.zip
elif command -v curl &> /dev/null; then
    curl -sL "$GITHUB_URL" -o include.zip
else
    echo "ERROR: wget o curl no disponibles"
    exit 1
fi

# Extraer
echo "  Extrayendo..."
unzip -q include.zip

# Instalar
echo "  Instalando en $INSTALL_DIR..."
sudo mkdir -p "$INSTALL_DIR"
sudo cp -r nlohmann "$INSTALL_DIR/"

# Limpiar
rm -rf "$TEMP_DIR"

echo "OK: Instalacion completada"
echo ""
echo "Para verificar:"
echo "  g++ -I/usr/local/include -c -std=c++17 -E -dM - </dev/null | grep __cplusplus"
