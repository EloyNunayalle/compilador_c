# Stage 1: Build C++ compiler
FROM ubuntu:22.04 AS cpp-builder

RUN apt-get update && apt-get install -y \
    build-essential g++ make nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY *.cpp *.h Makefile ./
RUN make all

# Stage 2: Build frontend
FROM node:18-alpine AS frontend-builder

WORKDIR /app
COPY frontend/package*.json ./
RUN npm install
COPY frontend/ .
RUN npm run build

# Stage 3: Production
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    python3 python3-pip python3-venv nginx build-essential \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy compiled compilers from stage 1
COPY --from=cpp-builder /app/minicc /app/minicc_api ./

# Copy backend
COPY backend/src/backend.py ./
COPY backend/requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

# Copy built frontend from stage 2
COPY --from=frontend-builder /app/dist ./frontend/dist

# Copy nginx config and entrypoint
COPY nginx.conf /etc/nginx/nginx.conf
COPY entrypoint.sh /entrypoint.sh
RUN sed -i 's/\r$//' /entrypoint.sh && chmod +x /entrypoint.sh

EXPOSE 80

CMD ["/entrypoint.sh"]
