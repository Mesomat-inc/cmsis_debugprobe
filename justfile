in_nix := env_var_or_default("IN_NIX_SHELL", "")

default: help

help:
    @just --list

_check_nix_shell:
    @if [ -z "{{ in_nix }}" ]; then \
        echo  "\033[33mWarning: Not in a Nix shell. Consider using 'nix develop'.\033[0m"; \
    fi

[working-directory("./")]
_check_init:
    @if git submodule status --recursive | grep -q '^-'; then \
        echo "\033[33mSubmodules not initialized, running 'just init'...\033[0m"; \
        just init; \
    fi

# Initialize git submodules (FreeRTOS)
[working-directory("./")]
init:
    git submodule update --init --recursive

# Build the project
[working-directory("./")]
build: _check_nix_shell _check_init
    echo "Building the project..."; \
    [ -d build ] || mkdir build; \
    cd build && cmake .. && make; \
    echo "Build completed successfully. use build/debugprobe.uf2 to flash the rp2040 board."

# Build Pico firmware for sensor programming
[working-directory("./")]
build-sensor: _check_nix_shell _check_init
    set -e; \
    echo "Building sensor debug probe firmware..."; \
    [ -d build-sensor ] || mkdir -p build-sensor; \
    cd build-sensor; \
    cmake -DPROBE_ROLE=sensor ..; \
    make; \
    echo "Build completed successfully. Use build-sensor/debugprobe.uf2 to flash the sensor Picos"

# Build Pico firmware for gateway programming
[working-directory("./")]
build-gateway: _check_nix_shell _check_init
    set -e; \
    echo "Building gateway debug probe firmware..."; \
    [ -d build-gateway ] || mkdir -p build-gateway; \
    cd build-gateway; \
    cmake -DPROBE_ROLE=gateway ..; \
    make; \
    echo "Build completed successfully. Use build-gateway/debugprobe.uf2 to flash the gateway Picos"
