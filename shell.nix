{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    (python3.withPackages(ps: with ps; [
      evdev
      pysdl2
    ]))
    SDL2
    socat
  ];
}
