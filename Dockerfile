# Dockerfile for backend: C++ build tools + Python + Uvicorn

# Start from an official Python image with build-essential for C++
FROM buildpack-deps:bullseye

# Install system dependencies: build-essential for C++ compilation, and any useful tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    libgsl-dev \
    libtiff-dev \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# Set workdir
WORKDIR /app

# Copy the rest of the codebase
COPY . .

# Build the C++ project (as in CI)
RUN rm -rf build && mkdir -p build && cd build && cmake .. && make -j4 && cd ..

# Expose port for backend communication
EXPOSE 8000

# Default command to run the main binary (can be overridden)
CMD ["./build/viss-release", "test_config1.txt", "0", "opt", "-o"]