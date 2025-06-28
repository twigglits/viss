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

# Copy the rest of the codebase
COPY . .
