{ pkgs, ...}:
let
  networkConfig = import ./network-config.nix;
in
{
  networking = {
    hostName = "dnd-soundboard";
    firewall.enable = true;
    nftables.enable = true;
  };

  services = {
    # Enable OpenSSH out of the box.
    sshd.enable = true;

    # Avahi settings for mDNS
    avahi = {
      enable = true;
      nssmdns4 = true;
      publish = {
        enable = true;
        addresses = true;
        workstation = true;
      };
      openFirewall = true;
    };

    # Use pulseaudio for audio server
    pulseaudio.enable = true;

    getty.autologinUser = "admin";
  };

  users.users.admin = {
    isNormalUser = true;
    home = "/home/admin";
    description = "Administrator";
    extraGroups = ["wheel" "dialout"];
    openssh.authorizedKeys.keys = networkConfig.publicKeys;
  };

  security.sudo = {
    enable = true;
    wheelNeedsPassword = false;
  };

  environment.systemPackages = with pkgs; [
    # Soundboard dependencies
    (python3.withPackages(ps: with ps; [
      evdev
      pysdl2
    ]))
    SDL2
    socat

    # Normal packages
    git
    htop
    killall
    neofetch
    pciutils
    screen
    usbutils
    vim
    wget
    alsa-utils
  ];
}
