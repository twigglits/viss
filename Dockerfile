# Dockerfile for backend: C++ build tools + Python + Uvicorn

# Start from an official Python image with build-essential for C++
FROM python:3.11-slim

# Install system dependencies: build-essential for C++ compilation, and any useful tools
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        g++ \
        cmake \
        git \
        && rm -rf /var/lib/apt/lists/*

# Set workdir
WORKDIR /app

# Copy requirements and install Python dependencies (if any)
COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

# Copy the rest of the codebase
COPY . .

# Expose the port uvicorn will run on (default 8000)
EXPOSE 8000

# Default command (can be overridden)
CMD ["uvicorn", "app:app", "--host", "0.0.0.0", "--port", "8000"]
