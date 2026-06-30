{ pkgs ? import <nixpkgs> {} }:
let
  soundboard_package = pkgs.callPackage ./dnd-soundboard.nix {};
in
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    argparse
    catch2_3
    clang-tools
    cmake
    git
    libevdev
    pkg-config
    sdbus-cpp
    sdl3
    sdl3-mixer
  ];
}
