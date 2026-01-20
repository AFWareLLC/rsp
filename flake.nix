{
  description = "rsp";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
  };

  outputs = {
    self,
    nixpkgs,
  }: let
    supportedSystems = [
      "x86_64-linux"
      "aarch64-linux"
      "aarch64-darwin"
    ];

    forAllSystems = f:
      nixpkgs.lib.genAttrs supportedSystems (
        system:
          f {
            pkgs = import nixpkgs {inherit system;};
          }
      );
  in {
    formatter = forAllSystems ({pkgs}: pkgs.alejandra);
    devShells = forAllSystems ({pkgs}: {
      default = pkgs.mkShell {
        packages = with pkgs; [
          clang
          clang-tools
          flatbuffers
          go
          alejandra
          statix
          deadnix

          (writeShellApplication {
            name = "rsp-fmt-include";
            runtimeInputs = [clang-tools findutils];
            text = ''
              set -euo pipefail

              if [ ! -d include ]; then
                echo "No ./include directory found." >&2
                exit 1
              fi

              echo "clang-format: formatting ./include..."

              find include \
                -path include/afware/rsp/third_party -prune -o \
                -name scope_info_generated.h -prune -o \
                -type f \
                \( -name '*.h' -o -name '*.hh' -o -name '*.hpp' -o -name '*.hxx' -o -name '*.inc' \
                   -o -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' \) \
                -print0 \
              | xargs -0 -r clang-format -i

              echo "Done."
            '';
          })
        ];

        shellHook = ''
          export CC=clang
          export CXX=clang++
          export NIX_ENFORCE_NO_NATIVE=0
        '';
      };
    });
  };
}
