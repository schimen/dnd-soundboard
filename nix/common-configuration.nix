deviceName: sinkName: sourceName: { pkgs, ...}:
let
  networkConfig = import ./network-config.nix;
  userName = "soundplayer";
  userDir = "/home/${userName}";
  sampleDir = "${userDir}/sampledir";
  serviceConfig = import ./dnd-soundboard-service.nix userName sampleDir deviceName;
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
        domain = true;
        workstation = true;
      };
      openFirewall = true;
    };

    # Use pipewire for audio server
    pipewire = {
      enable = true;
      alsa.enable = true;
      alsa.support32Bit = true;
      pulse.enable = true;
      systemWide = true;
      wireplumber = {
        enable = true;
        # Set priority of requested sink and source higher than other devices
        extraConfig."default-sink-and-source" = {
          "monitor.alsa.rules" = [ 
            {
              matches = [ { "node.name" = sinkName; } ];
              actions = {
                "update-props" = {
                  "priority.driver" = 1234;
                  "priority.session" = 1234;
                };
              };
            } {
              matches = [ { "node.name" = sourceName; } ];
              actions = {
                "update-props" = {
                  "priority.driver" = 2345;
                  "priority.session" = 2345;
                };
              };
            }
          ];
        };
        # Set default volume to 100% for both sink and source
        extraConfig."default-volume" = {
          "wireplumber.settings" = {
              "device.routes.default-sink-volume" = 1.0;
              "device.routes.default-source-volume" = 1.0;
          };
        };
      };
    };

    getty.autologinUser = userName;
  };

  users.users."${userName}" = {
    isNormalUser = true;
    home = userDir;
    extraGroups = ["wheel" "dialout" "input" "pipewire"];
    openssh.authorizedKeys.keys = networkConfig.publicKeys;
  };

  security = {
    # rtkit for pipewire
    rtkit.enable = true;

    # Passwordless sudo
    sudo = {
      enable = true;
      wheelNeedsPassword = false;
    };

    # Access to power off without password
    polkit = {
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
  };

  environment.systemPackages = with pkgs; [
    alsa-utils
    fastfetch
    vim
  ];

  # Make pipewire and sdl3 smaller
  nixpkgs.overlays = [
    (final: super: {
      pipewire = super.pipewire.override {
        vulkanSupport = false;
        bluezSupport = false;
        x11Support = false;
      };
      sdl3 = super.sdl3.override {
        drmSupport = false; 
        jackSupport = false;
        libdecorSupport = false;
        openglSupport = false;
        traySupport = false;
        vulkanSupport = false;
        waylandSupport = false;
        x11Support = false;
      };
    })
  ];
}
