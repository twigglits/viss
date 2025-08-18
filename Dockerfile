# Dockerfile for backend: C++ build tools + Python + Uvicorn

# Start from an official Python image with build-essential for C++
FROM buildpack-deps:bullseye

# Install system dependencies: build-essential for C++ compilation, and any useful tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    libgsl-dev \
    libtiff-dev \
    libboost-all-dev \
    libasio-dev \
    libhiredis-dev \
    libssl-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set workdir
WORKDIR /app

# Copy the rest of the codebase
COPY . .

# Build the C++ project (as in CI)
RUN rm -rf build && mkdir -p build && cd build && cmake .. && make -j4 redis++ viss-release && cd ..

# We invalidate cache for now as we are often updated viss-api
ARG CACHE_BREAKER=manual
RUN cd build && make -j4 viss-api && cd ..

# Expose port for backend communication
EXPOSE 8000

# Default command to run the main binary (can be overridden)
CMD ["./build/viss-api"]