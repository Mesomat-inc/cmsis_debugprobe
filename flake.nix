{
  description = "Development environment for Raspberry Pi Debugprobe";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forEachSystem = nixpkgs.lib.genAttrs supportedSystems;
    in
    {
      devShells = forEachSystem (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              cmake
              gnumake
              gcc-arm-embedded
              python3
              git
              pico-sdk
            ];

            # Automatically map the SDK path so CMake finds it without manual exports
            PICO_SDK_PATH = "${pkgs.pico-sdk}/lib/pico-sdk";

            shellHook = ''
              echo "🚀 debugprobe development shell active"
              echo "start using justfile commands: just help"
            '';
          };
        }
      );
    };
}
