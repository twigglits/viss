# Build and run the vrust API server (edition 2024 requires a recent Rust toolchain)

FROM rust:1.85 as builder
WORKDIR /work

# Pre-copy manifest for better caching
COPY Cargo.toml Cargo.toml
COPY src src

RUN cargo build --release --bin vrust_api

FROM debian:bookworm-slim
WORKDIR /app
COPY --from=builder /work/target/release/vrust_api /app/vrust_api

EXPOSE 8000
CMD ["/app/vrust_api"]
