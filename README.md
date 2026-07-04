# MiniC Compiler IDE

Compilador de C a x86-64 con IDE integrado (editor, AST visual, assembly, ejecución).

## Quick Start

```bash
docker compose up -d
```

Acceder a http://localhost

## Features

- **Code Editor** — Monaco Editor con resaltado de sintaxis C
- **AST Visualization** — Árbol interactivo con D3.js (zoom, colores por tipo)
- **Assembly Viewer** — x86-64 con números de línea
- **Output Panel** — Resultado de ejecución del programa compilado

## Project Structure

```
├── frontend/            React IDE
│   ├── src/components/  UI Components
│   ├── src/services/    HTTP Client
│   └── package.json
├── backend/
│   └── src/backend.py   FastAPI Backend
├── *.cpp *.h Makefile   Compilador C++ MiniC
├── Dockerfile           Contenedor multi-etapa (todo incluido)
├── docker-compose.yml   Orquestación (servicio único)
└── README.md
```
