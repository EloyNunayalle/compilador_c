"""
Cliente Python para la API del compilador MiniC
===============================================
Ejemplo de uso del backend
"""

import requests
import json
from typing import Optional

class MiniCClient:
    """Cliente para interactuar con la API del compilador MiniC"""

    def __init__(self, base_url: str = "http://localhost:8000"):
        self.base_url = base_url
        self.compile_endpoint = f"{base_url}/api/v1/compile"

    def compile(
        self,
        code: str,
        mode: str = "both",
        optimize: bool = True,
    ) -> dict:
        """
        Compila codigo C
        
        Args:
            code: Codigo C a compilar
            mode: "json" (AST), "asm" (assembly), "both" (ambos)
            optimize: Activar optimizaciones
            
        Returns:
            dict con resultado de compilacion
        """
        payload = {
            "code": code,
            "mode": mode,
            "optimize": optimize,
        }

        response = requests.post(self.compile_endpoint, json=payload)
        response.raise_for_status()
        return response.json()

    def compile_ast(self, code: str, optimize: bool = True) -> dict:
        """Compila y retorna solo el AST"""
        return self.compile(code, mode="json", optimize=optimize)

    def compile_asm(self, code: str, optimize: bool = True) -> dict:
        """Compila y retorna solo el assembly"""
        return self.compile(code, mode="asm", optimize=optimize)

    def compile_both(self, code: str, optimize: bool = True) -> dict:
        """Compila y retorna AST y assembly"""
        return self.compile(code, mode="both", optimize=optimize)

    def print_result(self, result: dict):
        """Imprime el resultado de forma legible"""
        if not result.get("success"):
            print(f"Error: {result.get('error')}")
            if result.get("details"):
                print(f"   Detalles: {result.get('details')}")
            return

        print(f"Compilacion exitosa (modo: {result.get('mode')})")
        print()

        if result.get("ast"):
            print("AST:")
            print(json.dumps(result["ast"], indent=2))
            print()

        if result.get("assembly"):
            print("Assembly x86-64 AT&T:")
            print(result["assembly"])


# ============================================================================
# Ejemplos de uso
# ============================================================================

if __name__ == "__main__":
    client = MiniCClient()

    # Ejemplo 1: Fibonacci
    fib_code = """
    int fib(int n) {
        if (n <= 1) return n;
        return fib(n - 1) + fib(n - 2);
    }

    int main() {
        printf("%d\\n", fib(10));
        return 0;
    }
    """

    print("=" * 70)
    print("Ejemplo 1: Fibonacci")
    print("=" * 70)
    result = client.compile_both(fib_code)
    client.print_result(result)

    # Ejemplo 2: Simple aritmetica
    simple_code = """
    int main() {
        int x = 5;
        int y = 3;
        int z = x + y;
        printf("%d\\n", z);
        return 0;
    }
    """

    print()
    print("=" * 70)
    print("Ejemplo 2: Aritmetica simple")
    print("=" * 70)
    result = client.compile_asm(simple_code)
    client.print_result(result)

    # Ejemplo 3: Arrays
    array_code = """
    int main() {
        int arr[5] = {1, 2, 3, 4, 5};
        int sum = 0;
        for (int i = 0; i < 5; i = i + 1) {
            sum = sum + arr[i];
        }
        printf("%d\\n", sum);
        return 0;
    }
    """

    print()
    print("=" * 70)
    print("Ejemplo 3: Arrays")
    print("=" * 70)
    result = client.compile_ast(array_code)
    client.print_result(result)
