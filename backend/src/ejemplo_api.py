#!/usr/bin/env python3
"""
ejemplo_api.py - Ejemplos de uso de la API MiniC desde Python
==============================================================
Muestra diferentes formas de compilar y procesar resultados
"""

import requests
import json
from typing import Dict, Any


def ejemplo_1_simple():
    """Ejemplo simple: compilar una funcion"""
    print("\n" + "=" * 70)
    print("Ejemplo 1: Compilacion simple")
    print("=" * 70)

    code = """
    int main() {
        int x = 5;
        int y = 3;
        int z = x + y;
        printf("%d\\n", z);
        return 0;
    }
    """

    response = requests.post(
        "http://localhost:8000/api/v1/compile",
        json={"code": code, "mode": "both", "optimize": True},
    )

    result = response.json()
    if result["success"]:
        print("\nCompilacion exitosa!")
        print(f"Modo: {result['mode']}")
        if result.get("ast"):
            print(f"\nAST (primeras 200 chars): {str(result['ast'])[:200]}...")
        if result.get("assembly"):
            print(f"\nAssembly (primeras 300 chars):\n{result['assembly'][:300]}...")
    else:
        print(f"Error: {result['error']}")


def ejemplo_2_solo_ast():
    """Ejemplo: obtener solo el AST"""
    print("\n" + "=" * 70)
    print("Ejemplo 2: Extraer solo AST")
    print("=" * 70)

    code = "int main() { int arr[3] = {1, 2, 3}; return 0; }"

    response = requests.post(
        "http://localhost:8000/api/v1/compile",
        json={"code": code, "mode": "json"},
    )

    result = response.json()
    if result["success"]:
        print("\nAST extraido:")
        print(json.dumps(result["ast"], indent=2)[:500] + "...")
    else:
        print(f"Error: {result['error']}")


def ejemplo_3_solo_asm():
    """Ejemplo: obtener solo assembly"""
    print("\n" + "=" * 70)
    print("Ejemplo 3: Extraer solo Assembly")
    print("=" * 70)

    code = """
    int add(int a, int b) {
        return a + b;
    }

    int main() {
        int result = add(5, 3);
        printf("%d\\n", result);
        return 0;
    }
    """

    response = requests.post(
        "http://localhost:8000/api/v1/compile",
        json={"code": code, "mode": "asm"},
    )

    result = response.json()
    if result["success"]:
        print("\nAssembly generado:")
        print(result["assembly"])
    else:
        print(f"Error: {result['error']}")


def ejemplo_4_fibonacci():
    """Ejemplo: Fibonacci con analisis del AST"""
    print("\n" + "=" * 70)
    print("Ejemplo 4: Fibonacci - Analisis del AST")
    print("=" * 70)

    code = """
    int fib(int n) {
        if (n <= 1) return n;
        return fib(n - 1) + fib(n - 2);
    }

    int main() {
        printf("%d\\n", fib(10));
        return 0;
    }
    """

    response = requests.post(
        "http://localhost:8000/api/v1/compile",
        json={"code": code, "mode": "both", "optimize": True},
    )

    result = response.json()
    if result["success"]:
        print("\nFibonacci compilado!")

        # Analizar el AST
        if result.get("ast") and result["ast"].get("functions"):
            print(f"\nFunciones encontradas:")
            for func in result["ast"]["functions"]:
                print(f"  - {func['name']}() -> {func['returnType']['base']}")

        # Mostrar assembly
        if result.get("assembly"):
            asm_lines = result["assembly"].split("\n")
            print(f"\nAssembly: {len(asm_lines)} lineas generadas")
            print("  Primeras lineas:")
            for line in asm_lines[:10]:
                if line.strip():
                    print(f"    {line}")
    else:
        print(f"Error: {result['error']}")


def ejemplo_5_manejo_errores():
    """Ejemplo: manejo de errores"""
    print("\n" + "=" * 70)
    print("Ejemplo 5: Manejo de errores")
    print("=" * 70)

    # Codigo con error semantico
    code = "int main() { printf(\"%d\\n\", undefined_variable); return 0; }"

    response = requests.post(
        "http://localhost:8000/api/v1/compile",
        json={"code": code, "mode": "json"},
    )

    result = response.json()
    print(f"\nCompilando codigo con error...")
    print(f"Success: {result['success']}")
    if not result["success"]:
        print(f"Error: {result['error']}")
        if result.get("details"):
            print(f"Detalles: {result['details']}")


def ejemplo_6_comparar_optimizaciones():
    """Ejemplo: comparar con y sin optimizaciones"""
    print("\n" + "=" * 70)
    print("Ejemplo 6: Comparacion de optimizaciones")
    print("=" * 70)

    code = """
    int main() {
        int x = 5 + 3;
        int y = 2 * 4;
        int z = x + y;
        return z;
    }
    """

    # Sin optimizar
    response1 = requests.post(
        "http://localhost:8000/api/v1/compile",
        json={"code": code, "mode": "asm", "optimize": False},
    )
    result1 = response1.json()
    asm1 = result1.get("assembly", "").split("\n") if result1["success"] else []

    # Con optimizar
    response2 = requests.post(
        "http://localhost:8000/api/v1/compile",
        json={"code": code, "mode": "asm", "optimize": True},
    )
    result2 = response2.json()
    asm2 = result2.get("assembly", "").split("\n") if result2["success"] else []

    print(f"\nComparacion de optimizaciones:")
    print(f"  Sin optimizar (-O0): {len(asm1)} lineas")
    print(f"  Con optimizar (-O1): {len(asm2)} lineas")
    print(f"  Reduccion: {len(asm1) - len(asm2)} lineas")
    if result1["success"] and result2["success"]:
        print(f"\nEficiencia: {100 * (len(asm1) - len(asm2)) / len(asm1):.1f}% menos codigo")


def ejemplo_7_procesamiento_json():
    """Ejemplo: procesamiento del JSON del AST"""
    print("\n" + "=" * 70)
    print("Ejemplo 7: Procesamiento del JSON del AST")
    print("=" * 70)

    code = """
    int main() {
        int x = 10;
        int y = x + 5;
        return y;
    }
    """

    response = requests.post(
        "http://localhost:8000/api/v1/compile",
        json={"code": code, "mode": "json"},
    )

    result = response.json()
    if result["success"]:
        ast = result["ast"]

        def contar_nodos(node: Dict[str, Any], tipo: str = None) -> int:
            """Cuenta nodos de un tipo especifico en el AST"""
            if not isinstance(node, dict):
                return 0

            count = 0
            if tipo is None or node.get("type") == tipo:
                count = 1

            for value in node.values():
                if isinstance(value, dict):
                    count += contar_nodos(value, tipo)
                elif isinstance(value, list):
                    for item in value:
                        if isinstance(item, dict):
                            count += contar_nodos(item, tipo)

            return count

        print(f"\nEstadisticas del AST:")
        print(f"  Total de nodos: {contar_nodos(ast)}")
        print(f"  VarDecStm: {contar_nodos(ast, 'VarDecStm')}")
        print(f"  AssignStm: {contar_nodos(ast, 'AssignStm')}")
        print(f"  BinaryExp: {contar_nodos(ast, 'BinaryExp')}")
        print(f"  ReturnStm: {contar_nodos(ast, 'ReturnStm')}")


if __name__ == "__main__":
    print("Ejemplos de uso de la API MiniC")
    print("====================================")

    try:
        # Verificar que el servidor esta activo
        response = requests.get("http://localhost:8000/api/v1/status", timeout=2)
        if response.status_code != 200:
            raise Exception("Server not responding correctly")

        # Ejecutar ejemplos
        ejemplo_1_simple()
        ejemplo_2_solo_ast()
        ejemplo_3_solo_asm()
        ejemplo_4_fibonacci()
        ejemplo_5_manejo_errores()
        ejemplo_6_comparar_optimizaciones()
        ejemplo_7_procesamiento_json()

        print("\n" + "=" * 70)
        print("Todos los ejemplos completados!")
        print("=" * 70)

    except requests.exceptions.ConnectionError:
        print("\nError: No se pudo conectar a http://localhost:8000")
        print("   Asegurate de que el backend esta ejecutandose:")
        print("   python backend.py")
    except Exception as e:
        print(f"\nError: {e}")
