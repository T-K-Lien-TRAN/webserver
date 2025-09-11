FROM ubuntu:latest
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    software-properties-common \
    build-essential \
    curl \
    cmake \
    && add-apt-repository ppa:ubuntu-toolchain-r/test \
    && apt-get update && apt-get install -y \
    g++-13 \
    && ln -sf /usr/bin/g++-13 /usr/bin/g++ \
    && ln -sf /usr/bin/gcc-13 /usr/bin/gcc

	WORKDIR /app