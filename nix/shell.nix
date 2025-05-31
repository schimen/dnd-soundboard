{ pkgs ? import <nixpkgs> {} }:
let
  soundboard_package = pkgs.callPackage ./dnd-soundboard.nix {};
in
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    socat
    soundboard_package
  ];
}
