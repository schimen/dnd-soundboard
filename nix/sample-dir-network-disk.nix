userName: sampleDir: { config, pkgs, ... }:
{
  services = {
    # Add Samba so that sample dir is browseable ofver network
    samba = {
      package = pkgs.samba4Full;
      enable = true;
      openFirewall = true;
      settings = {
        global = {
          "workgroup" = "WORKGROUP";
          "server string" = "smbnix";
          "netbios name" = "smbnix";
          "security" = "user";
          "hosts allow" = "192.168.0. 127.0.0.1 localhost";
          "hosts deny" = "0.0.0.0/0";
          "guest account" = "nobody";
          "map to guest" = "bad user";
        };
        "Samples" = {
          "path" = "${sampleDir}/";
          "browseable" = "yes";
          "writeable" = "yes";
          "read only" = "no";
          "guest ok" = "yes";
          "create mask" = "0644";
          "directory mask" = "0755";
        };
      };
    };
    samba-wsdd = {
      # make shares visible for Windows clients
      enable = true;
      openFirewall = true;
    };
  };
}

