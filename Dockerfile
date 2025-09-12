FROM ubuntu:24.04

# Disable interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y siege curl strace\
    build-essential \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
# Copy your binaries and config
COPY app/webserv .
COPY app/config_files/evaluation.conf ./evaluation.conf
# Make sure they’re executable
RUN chmod +x webserv

# Start the server and run Siege test
CMD bash -c "./webserv evaluation.conf"
