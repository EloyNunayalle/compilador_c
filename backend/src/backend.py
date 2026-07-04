"""
Backend FastAPI para el compilador MiniC
========================================
- Recibe codigo C
- Lo pasa al compilador C++ (minicc_api)
- Retorna AST en JSON y x86-64 assembly
"""

import subprocess
import json
import os
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import Optional, Dict, Any

# ============================================================================
# Configuracion
# ============================================================================

app = FastAPI(title="MiniC Compiler API", version="1.0.0")

# CORS habilitado para desarrollo
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Ruta del compilador
COMPILER_PATH = os.path.join(os.path.dirname(__file__), "..", "..", "minicc_api")
if not os.path.exists(COMPILER_PATH):
    # Buscar en diferentes locaciones
    possible_paths = [
        "./minicc_api",
        "../minicc_api",
        "../../minicc_api",
        "/usr/local/bin/minicc_api",
        "/opt/minicc_api",
    ]
    for path in possible_paths:
        if os.path.exists(path):
            COMPILER_PATH = path
            break


# ============================================================================
# Modelos Pydantic
# ============================================================================

class CompileRequest(BaseModel):
    """Request para compilar codigo C"""
    code: str
    mode: Optional[str] = "both"  # "json", "asm", "both"
    optimize: Optional[bool] = True


class CompileResponse(BaseModel):
    """Response de la compilacion"""
    success: bool
    mode: str
    ast: Optional[Dict[str, Any]] = None
    assembly: Optional[str] = None
    output: Optional[str] = None
    error: Optional[str] = None
    details: Optional[str] = None


# ============================================================================
# Endpoints
# ============================================================================

@app.get("/", tags=["Info"])
async def root():
    """Root endpoint - informacion del compilador"""
    return {
        "name": "MiniC Compiler API",
        "version": "1.0.0",
        "description": "Compilador de C mini a x86-64",
        "endpoints": {
            "compile": "/api/v1/compile (POST)",
            "status": "/api/v1/status (GET)",
        },
    }


@app.get("/api/v1/status", tags=["Status"])
async def status():
    """Estado del compilador"""
    compiler_ok = os.path.exists(COMPILER_PATH) and os.access(COMPILER_PATH, os.X_OK)
    return {
        "status": "ok" if compiler_ok else "error",
        "compiler_path": COMPILER_PATH,
        "compiler_exists": os.path.exists(COMPILER_PATH),
        "compiler_executable": os.access(COMPILER_PATH, os.X_OK) if os.path.exists(COMPILER_PATH) else False,
    }


@app.post("/api/v1/compile", response_model=CompileResponse, tags=["Compilation"])
async def compile_code(request: CompileRequest) -> CompileResponse:
    """
    Compila codigo C y retorna AST y/o x86-64 assembly.
    
    Parametros:
    - `code`: Codigo C a compilar
    - `mode`: "json" (solo AST), "asm" (solo assembly), "both" (AST + assembly)
    - `optimize`: True para activar optimizaciones
    
    Retorna:
    - `success`: True si la compilacion fue exitosa
    - `ast`: AST en formato JSON (si mode="json" o "both")
    - `assembly`: Codigo x86-64 en AT&T syntax (si mode="asm" o "both")
    - `error`: Mensaje de error (si success=False)
    """

    # Validar que el compilador exista
    if not os.path.exists(COMPILER_PATH):
        raise HTTPException(
            status_code=500,
            detail=f"Compilador no encontrado en {COMPILER_PATH}. "
                   f"Asegurate de compilar con: make",
        )

    # Validar el codigo de entrada
    if not request.code or not request.code.strip():
        raise HTTPException(
            status_code=400,
            detail="El codigo C no puede estar vacio",
        )

    # Validar modo
    valid_modes = ["json", "asm", "both"]
    if request.mode not in valid_modes:
        raise HTTPException(
            status_code=400,
            detail=f"Modo invalido. Debe ser uno de: {', '.join(valid_modes)}",
        )

    # Construir comando
    cmd = [COMPILER_PATH, "--mode", request.mode]
    if not request.optimize:
        cmd.append("-O0")
    else:
        cmd.append("-O1")

    try:
        # Ejecutar compilador
        process = subprocess.run(
            cmd,
            input=request.code.encode("utf-8"),
            capture_output=True,
            timeout=10,
        )

        # Parsear salida JSON del compilador
        try:
            output_json = json.loads(process.stdout.decode("utf-8"))
        except json.JSONDecodeError as e:
            return CompileResponse(
                success=False,
                mode=request.mode,
                error="Invalid JSON response from compiler",
                details=str(e),
            )

        # Si el compilador reporto error
        if not output_json.get("success", False):
            return CompileResponse(
                success=False,
                mode=request.mode,
                error=output_json.get("error", "Unknown error"),
                details=output_json.get("details"),
            )

        # Compilacion exitosa
        response_data = CompileResponse(
            success=True,
            mode=request.mode,
            ast=output_json.get("ast"),
            assembly=output_json.get("assembly"),
        )

        # Ejecutar el assembly generado
        assembly_text = output_json.get("assembly")
        if assembly_text:
            import tempfile, shutil
            tmp_dir = tempfile.mkdtemp()
            asm_path = os.path.join(tmp_dir, "program.s")
            exe_path = os.path.join(tmp_dir, "program")
            try:
                with open(asm_path, "w") as f:
                    f.write(assembly_text)
                compile_proc = subprocess.run(
                    ["gcc", "-o", exe_path, asm_path],
                    capture_output=True, timeout=10
                )
                if compile_proc.returncode == 0:
                    run_proc = subprocess.run(
                        [exe_path],
                        capture_output=True, timeout=5
                    )
                    stdout = run_proc.stdout.decode("utf-8", errors="replace")
                    stderr = run_proc.stderr.decode("utf-8", errors="replace")
                    lines = []
                    if stdout:
                        lines.append(stdout.rstrip())
                    if stderr:
                        lines.append("STDERR: " + stderr.rstrip())
                    if run_proc.returncode != 0:
                        lines.append(f"Exit code: {run_proc.returncode}")
                    if run_proc.returncode == -11:
                        import shutil
                        alt_path = os.path.join(tmp_dir, "alt")
                        shutil.copy(asm_path, alt_path + ".s")
                        # Try with -no-pie
                        subprocess.run(
                            ["gcc", "-no-pie", "-o", alt_path, alt_path + ".s"],
                            capture_output=True, timeout=10
                        )
                        alt_run = subprocess.run(
                            [alt_path], capture_output=True, timeout=5
                        )
                        err_detail = f" (alt: rc={alt_run.returncode}, out={alt_run.stdout.decode(errors='replace')[:100]})" if os.path.exists(alt_path) else ""
                    else:
                        err_detail = ""
                    lines.append(f"Exit code: {run_proc.returncode}{err_detail}")
                    response_data.output = "\n".join(lines) if lines else "(ejecucion sin salida)"
                else:
                    err = compile_proc.stderr.decode("utf-8", errors="replace")
                    response_data.output = f"(error al ensamblar:\n{err})"
            except subprocess.TimeoutExpired:
                response_data.output = "(ejecucion cancelada: timeout 5s)"
            except Exception as e:
                response_data.output = f"(error en ejecucion: {e})"
            finally:
                shutil.rmtree(tmp_dir, ignore_errors=True)

        return response_data

    except subprocess.TimeoutExpired:
        raise HTTPException(
            status_code=408,
            detail="La compilacion tardo demasiado tiempo (timeout)",
        )
    except Exception as e:
        raise HTTPException(
            status_code=500,
            detail=f"Error al ejecutar el compilador: {str(e)}",
        )


@app.post("/api/v1/compile/ast", tags=["Compilation"])
async def compile_ast(request: CompileRequest):
    """Compila y retorna solo el AST en JSON"""
    request.mode = "json"
    result = await compile_code(request)
    return result


@app.post("/api/v1/compile/asm", tags=["Compilation"])
async def compile_asm(request: CompileRequest):
    """Compila y retorna solo el codigo x86-64 assembly"""
    request.mode = "asm"
    result = await compile_code(request)
    return result


# ============================================================================
# Manejo de errores
# ============================================================================

@app.exception_handler(HTTPException)
async def http_exception_handler(request, exc):
    return {
        "success": False,
        "mode": "error",
        "error": exc.detail,
    }


@app.exception_handler(Exception)
async def general_exception_handler(request, exc):
    return {
        "success": False,
        "mode": "error",
        "error": str(exc),
    }


# ============================================================================
# Ejecucion
# ============================================================================

if __name__ == "__main__":
    import uvicorn

    uvicorn.run(
        app,
        host="0.0.0.0",
        port=8000,
        log_level="info",
    )
