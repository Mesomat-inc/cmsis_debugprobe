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
