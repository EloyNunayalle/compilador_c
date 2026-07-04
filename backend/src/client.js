// ============================================================================
// client.js - Cliente JavaScript para la API MiniC
// ============================================================================
// Uso en Node.js: node client.js
// Uso en navegador: <script src="client.js"></script>
// ============================================================================

class MiniCClient {
  constructor(baseUrl = "http://localhost:8000") {
    this.baseUrl = baseUrl;
    this.compileEndpoint = `${baseUrl}/api/v1/compile`;
  }

  async compile(code, mode = "both", optimize = true) {
    const payload = {
      code: code,
      mode: mode,
      optimize: optimize,
    };

    try {
      const response = await fetch(this.compileEndpoint, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify(payload),
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      return await response.json();
    } catch (error) {
      console.error("Error al compilar:", error);
      return {
        success: false,
        error: error.message,
      };
    }
  }

  async compileAst(code, optimize = true) {
    return this.compile(code, "json", optimize);
  }

  async compileAsm(code, optimize = true) {
    return this.compile(code, "asm", optimize);
  }

  async compileBoth(code, optimize = true) {
    return this.compile(code, "both", optimize);
  }

  printResult(result) {
    if (!result.success) {
      console.error(`Error: ${result.error}`);
      if (result.details) {
        console.error(`   Detalles: ${result.details}`);
      }
      return;
    }

    console.log(
      `Compilacion exitosa (modo: ${result.mode})`
    );
    console.log("");

    if (result.ast) {
      console.log("AST:");
      console.log(JSON.stringify(result.ast, null, 2));
      console.log("");
    }

    if (result.assembly) {
      console.log("Assembly x86-64 AT&T:");
      console.log(result.assembly);
    }
  }
}

// ============================================================================
// Ejemplos de uso
// ============================================================================

// En Node.js
if (typeof module !== "undefined" && module.exports) {
  module.exports = MiniCClient;

  // Si se ejecuta directamente
  if (require.main === module) {
    (async () => {
      const client = new MiniCClient("http://localhost:8000");

      // Ejemplo 1
      console.log("=".repeat(70));
      console.log("Ejemplo 1: Fibonacci");
      console.log("=".repeat(70));
      let code = `
        int fib(int n) {
          if (n <= 1) return n;
          return fib(n - 1) + fib(n - 2);
        }
        int main() {
          printf("%d\\n", fib(10));
          return 0;
        }
      `;
      let result = await client.compileBoth(code);
      client.printResult(result);

      // Ejemplo 2
      console.log("\n" + "=".repeat(70));
      console.log("Ejemplo 2: Variables");
      console.log("=".repeat(70));
      code = "int main() { int x = 5; return x; }";
      result = await client.compileAsm(code);
      client.printResult(result);
    })();
  }
}

// En navegador
else if (typeof window !== "undefined") {
  window.MiniCClient = MiniCClient;

  // Ejemplo: ejecutar al cargar la pagina
  document.addEventListener("DOMContentLoaded", async () => {
    const client = new MiniCClient("http://localhost:8000");

    // Usar la API desde la consola del navegador
    window.minicc = client;
    console.log("Cliente MiniC disponible como 'minicc'");
    console.log("Uso: minicc.compile('int main() { return 0; }', 'both')");
  });
}
