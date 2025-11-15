{
  description = "Video Thumbnailer";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";

    logmich.url = "github:logmich/logmich";
    logmich.inputs.nixpkgs.follows = "nixpkgs";
    logmich.inputs.tinycmmc.follows = "tinycmmc";
  };

  outputs = { self, nixpkgs, flake-utils, # gstreamer-fix,
              tinycmmc, logmich }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        packages = rec {
          default = vidthumb;

          vidthumb = pkgs.stdenv.mkDerivation {
            pname = "vidthumb";
            version = "0.0.0";

            src = ./.;

            doCheck = true;

            nativeBuildInputs = with pkgs; [
              cmake
              pkg-config
            ];

            buildInputs = with pkgs; [
              cairomm
              fmt
              gst_all_1.gstreamermm
            ] ++ [
              tinycmmc.packages.${system}.default
              logmich.packages.${system}.default
            ];

            propagatedBuildInputs = with pkgs; [
              gst_all_1.gst-plugins-good
              gst_all_1.gst-plugins-bad
            ];

            checkInputs = with pkgs; [
              gtest
            ];
          };
        };
      }
    );
}
