{ pkgs, stdenv }:

stdenv.mkDerivation {
  name = "soundboard-cpp";
  version = "0.2.0";
  nativeBuildInputs = with pkgs; [ catch2_3 cmake pkg-config ];
  buildInputs = with pkgs; [ argparse libevdev sdbus-cpp sdl3 sdl3-mixer ];
  src = ../.;
  cmakeBuildDir = "build-${stdenv.targetPlatform.system}";
}
