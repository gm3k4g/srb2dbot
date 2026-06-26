# ── srb2dbot portable builder ─────────────────────────────
# Builds a binary compatible with Debian 12 (glibc 2.36).
# Use debian:bookworm so the binary links against older glibc
# instead of the newer one on the NixOS host.
# ───────────────────────────────────────────────────────────
FROM debian:bookworm AS builder

RUN apt-get update -qq && apt-get install -y -qq \
    build-essential cmake nlohmann-json3-dev \
    libssl-dev libopus-dev libz-dev pkg-config git ca-certificates \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Build D++ from source (not packaged in Debian 12)
RUN git clone --depth 1 --branch v10.1.5 https://github.com/brainboxdotcc/dpp.git /tmp/dpp \
    && cmake -S /tmp/dpp -B /tmp/dpp/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
    && cmake --build /tmp/dpp/build -j$(nproc) \
    && cmake --install /tmp/dpp/build \
    && rm -rf /tmp/dpp

COPY . /tmp/srb2dbot

RUN cmake -S /tmp/srb2dbot -B /tmp/srb2dbot/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /tmp/srb2dbot/build -j$(nproc)

# ── Runtime stage ─────────────────────────────────────────
FROM debian:bookworm-slim

RUN apt-get update -qq && apt-get install -y -qq \
    libssl3 libopus0 ca-certificates \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Copy D++ runtime libs from builder
COPY --from=builder /usr/local/lib/libdpp.so* /usr/local/lib/
COPY --from=builder /tmp/srb2dbot/build/srb2dbot /usr/local/bin/srb2dbot
COPY secret.json.default /etc/srb2dbot/secret.json.default

RUN ldconfig

ENTRYPOINT ["/usr/local/bin/srb2dbot"]
