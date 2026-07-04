#!/bin/bash
set -e

python3 /app/backend.py &

nginx -g "daemon off;"
