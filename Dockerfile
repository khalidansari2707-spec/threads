# Multi-stage build for MultiCoreX C++ Backend Server & Visualizer
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY CMakeLists.txt ./
COPY backend ./backend
COPY tests ./tests
COPY frontend ./frontend

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --config Release

# Production runner image
FROM ubuntu:22.04 AS runner
WORKDIR /app

RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/multicorex_server /app/multicorex_server
COPY --from=builder /app/frontend /app/frontend

EXPOSE 8080
ENV PORT=8080
CMD ["/app/multicorex_server", "--host", "0.0.0.0", "--port", "8080", "--web", "/app/frontend"]
