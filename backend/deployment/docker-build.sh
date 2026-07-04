#!/bin/bash
# docker-build.sh - Script para compilar la imagen Docker

docker build -t minicc-api:latest .
echo "Imagen compilada: minicc-api:latest"
echo ""
echo "Para ejecutar:"
echo "  docker run -p 8000:8000 minicc-api:latest"
echo ""
echo "O con docker-compose:"
echo "  docker-compose up -d"
