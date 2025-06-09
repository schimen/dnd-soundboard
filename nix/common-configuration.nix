{ pkgs, ...}:
let
  networkConfig = import ./network-config.nix;
  userName = "soundplayer";
  userDir = "/home/${userName}";
  sampleDir = "${userDir}/sampledir";
  serviceConfig = import ./dnd-soundboard-service.nix userName sampleDir;
  sambaConfig = import ./sample-dir-network-disk.nix userName sampleDir;
in
{
  imports = [
    serviceConfig
    sambaConfig
  ];
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

    getty.autologinUser = userName;
  };

  users.users."${userName}" = {
    isNormalUser = true;
    home = userDir;
    extraGroups = ["wheel" "dialout" "input"];
    openssh.authorizedKeys.keys = networkConfig.publicKeys;
  };

  security.sudo = {
    enable = true;
    wheelNeedsPassword = false;
  };
  security.polkit = {
    enable = true;
    extraConfig = ''
      polkit.addRule(function(action, subject) {
        if (
          subject.isInGroup("users")
            && (
              action.id == "org.freedesktop.login1.power-off" ||
              action.id == "org.freedesktop.login1.power-off-multiple-sessions"
            )
          )
        {
          return polkit.Result.YES;
        }
      });
    ''; 
  };

  environment.systemPackages = with pkgs; [
    # Normal packages
    alsa-utils
    git
    htop
    killall
    neofetch
    pciutils
    screen
    usbutils
    vim
    wget
  ];
}
