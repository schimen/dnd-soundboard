{ lib, modulesPath, pkgs, ...}:
let
  keyboardName = "QEMU Virtio Keyboard";
  sinkName = "alsa_output.pci-0000_00_0a.0.analog-stereo";
  sourceName = "alsa_input.pci-0000_00_0a.0.analog-stereo";
  commonConfig = import ./common-configuration.nix keyboardName sinkName sourceName;
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
