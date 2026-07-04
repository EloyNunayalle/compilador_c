# Backend MiniC Compiler API

Backend FastAPI para el compilador MiniC que proporciona una API REST para compilar codigo C y obtener AST completo en JSON o codigo x86-64 assembly.

## Estructura del proyecto

```
backend/
├── src/                       # Codigo fuente del backend
│   ├── backend.py            # Server FastAPI principal
│   ├── client.py             # Cliente Python de ejemplo
│   ├── ejemplo_api.py        # Ejemplos de uso avanzado
│   └── client.js             # Cliente JavaScript/Node.js
│
├── tests/                     # Tests y archivos de prueba
│   └── test_api.http         # 19 requests HTTP listas para probar
│
├── deployment/               # Archivos de deployment
│   ├── Dockerfile            # Para containerizacion
│   ├── docker-compose.yml    # Orchestracion Docker
│   └── docker-build.sh       # Script para build Docker
│
├── scripts/                  # Scripts de setup e instalacion
│   ├── setup_api.sh         # Setup Linux/macOS
│   ├── setup_api.bat        # Setup Windows
│   └── install_json_lib.sh  # Instala nlohmann/json
│
└── requirements.txt          # Dependencias Python
```

## Instalacion rapida

### Opcion A: Linux/macOS

```bash
cd backend
bash scripts/setup_api.sh
source ../venv/bin/activate
python src/backend.py
```

### Opcion B: Windows

```cmd
cd backend
scripts\setup_api.bat
python src\backend.py
```

### Opcion C: Docker

```bash
cd backend/deployment
docker-compose up -d
# API en http://localhost:8000
```

## Endpoints principales

```
POST /api/v1/compile         # Compilar codigo C
POST /api/v1/compile/ast     # Solo AST en JSON
POST /api/v1/compile/asm     # Solo x86-64 assembly
GET  /api/v1/status          # Estado del compilador
GET  /docs                   # Documentacion Swagger
```

## Uso

### Desde Python

```python
from src.client import MiniCClient

client = MiniCClient("http://localhost:8000")
result = client.compile_both("int main() { return 42; }")
print(result["ast"])
print(result["assembly"])
```

### Desde JavaScript

```javascript
const client = new MiniCClient("http://localhost:8000");
const result = await client.compileBoth("int main() { return 42; }");
console.log(result.ast);
console.log(result.assembly);
```

### Desde cURL

```bash
curl -X POST http://localhost:8000/api/v1/compile \
  -H 'Content-Type: application/json' \
  -d '{"code":"int main(){return 0;}","mode":"both"}'
```

## Archivos principales

### src/backend.py
- Server FastAPI
- 6 endpoints REST
- Manejo de errores
- CORS habilitado
- Validacion con Pydantic

### src/client.py
- Cliente Python para la API
- Metodos: compile(), compile_ast(), compile_asm(), compile_both()
- Ejemplos de uso

### src/ejemplo_api.py
- 7 ejemplos de casos de uso
- Compilacion simple
- Extracion de AST y assembly
- Comparacion de optimizaciones
- Procesamiento del JSON del AST

### tests/test_api.http
- 19 requests HTTP listas para probar
- Compatible con VSCode REST Client, Postman, Insomnia
- Ejemplos de todos los endpoints

## Dependencias Python

```
fastapi==0.104.1
uvicorn[standard]==0.24.0
pydantic==2.5.0
python-multipart==0.0.6
requests==2.31.0
```

## Requisitos del sistema

### Compilador
- gcc, g++, make (Linux/macOS)
- MinGW (Windows)
- nlohmann/json (se instala automaticamente)

### Python
- Python 3.8+
- pip

## Endpoints de compilacion

### POST /api/v1/compile

Request:
```json
{
  "code": "int main() { int x = 5; return x; }",
  "mode": "both",
  "optimize": true
}
```

Response:
```json
{
  "success": true,
  "mode": "both",
  "ast": { ... },
  "assembly": "..."
}
```

## Modos de compilacion

| Modo | AST | Assembly | Uso |
|------|-----|----------|-----|
| `json` | Si | No | Analisis AST |
| `asm` | No | Si | Ejecucion |
| `both` | Si | Si | Compilacion completa |

## Troubleshooting

### "Compilador no encontrado"
```bash
cd ..
make all
```

### "nlohmann/json no encontrado"
```bash
bash backend/scripts/install_json_lib.sh
```

### "Puerto 8000 en uso"
```bash
uvicorn src.backend:app --port 8001
```

### "ModuleNotFoundError: fastapi"
```bash
pip install -r backend/requirements.txt
```

## Archivos de prueba

Abre `tests/test_api.http` con VSCode REST Client extension o importalo en Postman/Insomnia.

## Desarrollo

Para desarrollo con recarga automatica:

```bash
pip install -r requirements.txt
uvicorn src.backend:app --reload
```

## Production

```bash
pip install gunicorn
gunicorn -w 4 -b 0.0.0.0:8000 src.backend:app
```

## Documentacion

- API interactiva: http://localhost:8000/docs
- Ejemplos: `python src/ejemplo_api.py`
- Tests: `tests/test_api.http`
