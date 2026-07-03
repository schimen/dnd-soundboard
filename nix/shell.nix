{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    # C++ dependencies
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

    # Python dependencies
    (python3.withPackages(ps: with ps; [
      evdev
      dbus-python
      pysdl2
    ]))
    SDL2
  ];
}
