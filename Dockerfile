FROM debian:bullseye

RUN apt-get update && apt-get install -y \
    build-essential \
    libncurses5-dev \
    libncursesw5-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Build both server and client
RUN make -C server && make -C client
