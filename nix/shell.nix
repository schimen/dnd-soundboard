{ pkgs ? import <nixpkgs> {} }:
let
  soundboard_package = pkgs.callPackage ./dnd-soundboard.nix {};
in
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    argparse
    clang-tools
    libevdev
    pkg-config
    sdbus-cpp
    SDL2
    socat
    soundboard_package
  ];
}
