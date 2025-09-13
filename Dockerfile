FROM ubuntu:24.04

# Disable interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y siege curl strace net-tools \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

