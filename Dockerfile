# syntax=docker/dockerfile:1

# Build and play inside a container:
#   docker build -t blackjack .
#   docker run -it --rm blackjack
#
# Export the Linux build as plain files (the executable plus assets/) into ./dist:
#   docker build --target export --output type=local,dest=dist .
#
# Export the image itself:
#   docker save blackjack | gzip > blackjack-image.tar.gz

# ---- build ----
FROM debian:bookworm-slim AS build
RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ make \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY Makefile main.cpp ./
COPY src ./src
# Link the C++ runtime statically so an exported binary doesn't depend on this image's libstdc++.
RUN make LDFLAGS="-static-libstdc++ -static-libgcc" \
    && strip blackjack

# ---- export: just the files, for `--output` (the game finds assets/ next to the executable) ----
FROM scratch AS export
COPY --from=build /src/blackjack /blackjack
COPY assets /assets

# ---- runtime: the default target ----
FROM debian:bookworm-slim AS runtime
ENV LANG=C.UTF-8
RUN useradd --create-home --shell /usr/sbin/nologin player
WORKDIR /app
COPY --from=build /src/blackjack ./blackjack
COPY assets ./assets
USER player
ENTRYPOINT ["./blackjack"]
