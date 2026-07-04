# MiniC Compiler IDE Frontend

Frontend React profesional para el compilador MiniC con interfaz estilo IDE.

## Características

- **Editor de código** - Editor Monaco con syntax highlighting para C
- **Visualización AST dinámica** - Árbol sintáctico interactivo con D3.js, zoom y pan
- **Generador x86-64** - Visualización de código assembly generado
- **Terminal de salida** - Panel de resultados y errores
- **Interfaz IDE** - Layout de 4 paneles similar a VSCode/Visual Studio
- **Compilación en tiempo real** - Compilar con Ctrl+Enter
- **Optimizaciones** - Soporte para flags -O0/-O1

## Instalación

### Desarrollo local

```bash
npm install
npm run dev
```

Abre: http://localhost:3000

### Producción

```bash
npm install
npm run build
npm run preview
```

## Requisitos

- Node.js 16+
- npm 8+
- Compilador MiniC API corriendo en http://localhost:8000

## Dependencias principales

```json
{
  "react": "^18.2.0",
  "@monaco-editor/react": "^4.5.0",
  "axios": "^1.6.0",
  "d3": "^7.8.0"
}
```

## Variables de entorno

```bash
REACT_APP_API_URL=http://localhost:8000/api/v1
```

## Estructura de archivos

```
frontend/
├── src/
│   ├── components/
│   │   ├── CodeEditor.jsx      Editor de codigo
│   │   ├── ASTVisualizer.jsx   Visualizador AST
│   │   ├── AssemblyViewer.jsx  Visualizador assembly
│   │   └── OutputPanel.jsx     Panel de salida
│   ├── services/
│   │   └── api.js              Servicio HTTP
│   ├── App.jsx                 Componente principal
│   └── main.jsx                Punto de entrada
├── public/
│   └── index.html
├── package.json
├── vite.config.js
└── Dockerfile
```

## Componentes

### CodeEditor
Editor Monaco con soporte para C, configuración de indentación, autoformato, etc.

### ASTVisualizer
Visualización interactiva del AST con D3.js:
- Nodos coloreados por tipo (statements, expressions, literals)
- Zoom y pan interactivo
- Líneas de conexión entre nodos

### AssemblyViewer
Visualización de código x86-64 AT&T assembly con numeración de líneas.

### OutputPanel
Panel de terminal con salida y mensajes de error.

## Atajos de teclado

| Atajo | Acción |
|-------|--------|
| Ctrl+Enter | Compilar código |
| Ctrl+S | Autoformato (en editor) |

## Temas

Usa tema oscuro tipo VSCode/IDE profesional con:
- Fondo oscuro (#1e1e1e)
- Colores de sintaxis profesionales
- Fuente monoespaciada (Fira Code)

## Comunicación con API

Usa axios para conectar con el backend en http://localhost:8000/api/v1

Endpoints consumidos:
- POST /compile - Compilar código
- GET /status - Estado del compilador

## Docker

```bash
docker build -t minicc-ide:latest .
docker run -p 3000:3000 -e REACT_APP_API_URL=http://localhost:8000/api/v1 minicc-ide:latest
```

## Docker Compose

Se incluye en docker-compose.yml en la raiz del proyecto:

```bash
docker-compose up -d
```

Accede a:
- Frontend: http://localhost:3000
- API Docs: http://localhost:8000/docs
