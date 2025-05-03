# Use a recent Ubuntu base
FROM ubuntu:22.04

# Avoid interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and X11 (headers + client libs)
RUN apt-get update && \
    apt-get install -y build-essential make cmake xorg libx11-dev

# Copy your terminal-emulator sources into the image
WORKDIR /usr/src/terminal-emulator
COPY . .

# Build iksTerm
WORKDIR /usr/src/terminal-emulator/iksTerm
# Clean previous build cache
RUN rm -rf bin build
RUN make && make install

# Build paraShell
WORKDIR /usr/src/terminal-emulator/paraShell
RUN chmod +x ./install.sh && ./install.sh

# By default, launch the GUI iksTerm with paraShell
ENTRYPOINT ["iksTerm", "--shell=/usr/bin/paraShell"]