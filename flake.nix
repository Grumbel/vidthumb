{
  description = "Video Thumbnailer";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.05";
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";

    logmich.url = "github:logmich/logmich";
    logmich.inputs.nixpkgs.follows = "nixpkgs";
    logmich.inputs.flake-utils.follows = "flake-utils";
    logmich.inputs.tinycmmc.follows = "tinycmmc";

    gstreamer-fix.url = "github:milahu/nixpkgs?ref=patch-22";
  };

  outputs = { self, nixpkgs, flake-utils, gstreamer-fix,
              tinycmmc, logmich }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        gstreamer-fix-pkgs = gstreamer-fix.legacyPackages.${system};
      in {
        packages = rec {
          default = vidthumb;

          vidthumb = pkgs.stdenv.mkDerivation {
            pname = "vidthumb";
            version = "0.0.0";

            src = nixpkgs.lib.cleanSource ./.;

            nativeBuildInputs = [
              pkgs.cmake
              pkgs.pkg-config
            ];

            buildInputs = [
              tinycmmc.packages.${system}.default
              logmich.packages.${system}.default

              pkgs.cairomm
              pkgs.fmt_8
              # pkgs.gst_all_1.gstreamermm
              gstreamer-fix-pkgs.gst_all_1.gstreamermm
            ];
          };
        };
      }
    );
}
