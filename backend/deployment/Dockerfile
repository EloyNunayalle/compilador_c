# Dockerfile para el backend del compilador MiniC
FROM ubuntu:22.04

# Instalar dependencias del sistema
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    make \
    nlohmann-json3-dev \
    python3 \
    python3-pip \
    python3-venv \
    && rm -rf /var/lib/apt/lists/*

# Crear directorio de trabajo
WORKDIR /app

# Copiar archivos del compilador
COPY *.cpp *.h Makefile ./

# Compilar compilador
RUN make all

# Copiar archivos del backend Python
COPY backend/src/backend.py ./
COPY backend/requirements.txt ./

# Instalar dependencias Python
RUN pip install --no-cache-dir -r requirements.txt

# Puerto del backend
EXPOSE 8000

# Comando por defecto
CMD ["python3", "backend.py"]
