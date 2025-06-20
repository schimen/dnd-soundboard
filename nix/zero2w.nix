{ lib, modulesPath, pkgs, ...}:
let
  dtOverlays = import ./dt-overlays-zero2w.nix;
  networkConfig = import ./network-config.nix;
  paSink = "alsa_output.platform-soc_sound.stereo-fallback";
  paSource = "alsa_output.platform-soc_sound.stereo-fallback.monitor";
in
{
  imports = [
    ./sd-image.nix
    ./common-configuration.nix
  ];

  # Some packages (ahci fail... this bypasses that) https://discourse.nixos.org/t/does-pkgs-linuxpackages-rpi3-build-all-required-kernel-modules/42509
  nixpkgs.overlays = [
    (final: super: {
      makeModulesClosure = x:
        super.makeModulesClosure (x // { allowMissing = true; });
    })
  ];

  nixpkgs.hostPlatform = "aarch64-linux";
  # ! Need a trusted user for deploy-rs.
  nix.settings.trusted-users = ["@wheel"];
  system.stateVersion = "25.05";

  zramSwap = {
    enable = true;
    algorithm = "zstd";
  };

  sdImage = {
    # bzip2 compression takes loads of time with emulation, skip it. Enable this if you're low on space.
    compressImage = false;
    imageName = "zero2.img";

    extraFirmwareConfig = {
      # Give up VRAM for more Free System Memory
      # - Disable camera which automatically reserves 128MB VRAM
      start_x = 0;
      # - Reduce allocation of VRAM to 16MB minimum for non-rotated (32MB for rotated)
      gpu_mem = 16;

      # Configure display to 800x600 so it fits on most screens
      # * See: https://elinux.org/RPi_Configuration
      hdmi_group = 2;
      hdmi_mode = 8;
    };
  };

  hardware = {
    bluetooth = {
      enable = true;
      powerOnBoot = true;
    };

    enableRedistributableFirmware = lib.mkForce false;
    firmware = [ pkgs.raspberrypiWirelessFirmware ]; # Keep this to make sure wifi works
    deviceTree.filter = "bcm2837-rpi-zero*.dtb";
    deviceTree.overlays = [ dtOverlays.googlevoicechat_soundcard ];
  };

  boot = {
    kernelPackages = pkgs.linuxPackages_rpi02w;
    initrd.availableKernelModules = ["xhci_pci" "usbhid" "usb_storage"];
    loader = {
      grub.enable = false;
      generic-extlinux-compatible.enable = true;
    };

    # Avoids warning: mdadm: Neither MAILADDR nor PROGRAM has been set. This will cause the `mdmon` service to crash.
    # See: https://github.com/NixOS/nixpkgs/issues/254807
    swraid.enable = lib.mkForce false;
  };

  networking = {
    interfaces."wlan0".useDHCP = true;
    wireless = {
      enable = true;
      userControlled.enable = true;
      interfaces = ["wlan0"];
      networks = networkConfig.networks;
    };
  };

  time.timeZone = "Europe/Oslo";
  i18n.defaultLocale = "en_US.UTF-8";
  console = {
    font = "Lat2-Terminus16";
    keyMap = "no";
  };

  # Speed up boot by not waiting for network interfaces to come online
  systemd.network.wait-online.timeout = 0;

  services = {
    # Set pulseaudio sink
    pulseaudio.extraConfig = ''
      load-module module-stream-restore restore_device=false
      set-default-sink ${paSink}
      set-default-source ${paSource}
      set-sink-volume ${paSink} 100%
      set-source-volume ${paSource} 100%
    '';
  };
  environment.systemPackages = with pkgs; [
    libraspberrypi
    raspberrypifw
    dtc
    wpa_supplicant
  ];
}
