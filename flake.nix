{
  description = "rsp";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
      ];

      forAllSystems = f:
        nixpkgs.lib.genAttrs supportedSystems (system:
          f {
            pkgs = import nixpkgs { inherit system; };
          }
        );
    in {
      devShells = forAllSystems ({ pkgs }: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            clang
            flatbuffers
            go
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
