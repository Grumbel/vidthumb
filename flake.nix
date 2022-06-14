{
  description = "Video Thumbnailer";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-21.11";
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "gitlab:grumbel/cmake-modules";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";

    logmich.url = "gitlab:logmich/logmich";
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
              tinycmmc.defaultPackage.${system}
              logmich.defaultPackage.${system}

              pkgs.cairomm
              pkgs.fmt
              # pkgs.gst_all_1.gstreamermm
              gstreamer-fix-pkgs.gst_all_1.gstreamermm
            ];
          };
        };
        defaultPackage = packages.vidthumb;
      });
}
