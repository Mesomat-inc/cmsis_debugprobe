in_nix := env_var_or_default("IN_NIX_SHELL", "")

default: help

help:
    @just --list


_check_nix_shell:
    @if [ -z "{{in_nix}}" ]; then \
        echo  "\033[33mWarning: Not in a Nix shell. Consider using 'nix develop'.\033[0m"; \
    fi

# Build the project
[working-directory: "./"]
build: _check_nix_shell
    echo "Building the project..."; \
    [ -d build ] || mkdir build; \
    cd build && cmake .. && make; \
    echo "Build completed successfully. use build/debugprobe.uf2 to flash the rp2040 board."

#Build Pico firmware for sensor programming
[working-directory: "./"]
build-sensor: _check_nix_shell
    set -e; \
    echo "Building sensor debug probe firmware..."; \
    [ -d build-sensor ] || mkdir -p build-sensor; \
    cd build-sensor; \
    cmake -DDEBUG_ON_PICO=ON -DPICO_BOARD=pico -DPROBE_ROLE=sensor -DPICOTOOL_FORCE_FETCH_FROM_GIT=ON ..; \
    make; \
    echo "Build completed successfully. Use build-sensor/debugprobe_on_pico.uf2 to flash the sensor Picos"

#Build Pico firmware for gateway programming
[working-directory: "./"]
build-gateway: _check_nix_shell
    set -e; \
    echo "Building gateway debug probe firmware..."; \
    [ -d build-gateway ] || mkdir -p build-gateway; \
    cd build-gateway; \
    cmake -DDEBUG_ON_PICO=ON -DPICO_BOARD=pico -DPROBE_ROLE=gateway -DPICOTOOL_FORCE_FETCH_FROM_GIT=ON ..; \
    make; \
    echo "Build completed successfully. Use build-gateway/debugprobe_on_pico.uf2 to flash the gateway Picos"





