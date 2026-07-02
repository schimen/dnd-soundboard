{ lib, modulesPath, pkgs, ...}:
let
  commonConfig = import ./common-configuration.nix "QEMU Virtio Keyboard";
in
{
  imports = [
    commonConfig
  ];

  time.timeZone = "Europe/Oslo";
  i18n.defaultLocale = "en_US.UTF-8";
  console = {
    font = "Lat2-Terminus16";
    keyMap = "no";
  };

  system.stateVersion = "26.05";
}
