{
  description = "Video Thumbnailer";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-21.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in rec {
        packages = flake-utils.lib.flattenTree {
          vidthumb = pkgs.stdenv.mkDerivation {
            pname = "vidthumb";
            version = "0.0.0";
            src = nixpkgs.lib.cleanSource ./.;
            nativeBuildInputs = [
              pkgs.cmake
              pkgs.pkg-config
            ];
            buildInputs = [
              pkgs.cairomm
              pkgs.fmt
              pkgs.gst_all_1.gstreamermm
            ];
          };
        };
        defaultPackage = packages.vidthumb;
      });
}
