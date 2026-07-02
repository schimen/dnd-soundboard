{ lib, modulesPath, pkgs, ...}:
{
  imports = [
    ./common-configuration.nix
  ];

  time.timeZone = "Europe/Oslo";
  i18n.defaultLocale = "en_US.UTF-8";
  console = {
    font = "Lat2-Terminus16";
    keyMap = "no";
  };

  system.stateVersion = "26.05";
}
