{ pkgs, python3, callPackage, ...}:
let
  soundboard_package = callPackage ./dnd-soundboard.nix {};
in
{
  users.extraUsers.soundplayer = {
    isSystemUser = true;
    createHome = true;
    home = "/home/soundboard";
  };
  systemd={
    soundplayer.sockets.play_sounds = {
      enable = true;
      description = "Socket for sending commands to play_sounds.service";
      listenStreams = [ "/tmp/play_sounds.socket" ];
    };
    soundplayer.services.play_sounds = {
      enable = true;
      description = "Service that plays sounds when commands are received";
      serviceConfig = {
        Type = "simple";
        ExecStart = "${python3}/bin/python3 ${soundboard_package}/bin/play_sounds.py";
        Sockets = "play_sounds.socket";
        StandardInput = "socket";
        StandardOutput = "journal";
        StandardError = "journal";
      };
    };
  };
}