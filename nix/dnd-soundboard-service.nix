{ pkgs, ...}:
let
  soundboard_package = pkgs.callPackage ./dnd-soundboard.nix {};
in
{
  systemd = {
    services.keyboard_controller = {
      serviceConfig = {
        Type = "simple";
        ExecStart = "${soundboard_package}/bin/keyboard_controller.py";
        StandardOutput="file:/tmp/play_sounds.stdin";
      };
    };
  };
  systemd.user = {
    sockets.play_sounds = {
      enable = true;
      description = "Socket for sending commands to play_sounds.service";
      socketConfig = {
        ListenFIFO = "/tmp/play_sounds.stdin";
      };
    };
    services.play_sounds = {
      enable = true;
      description = "Service that plays sounds when commands are received";
      serviceConfig = {
        Type = "simple";
        ExecStart = "${soundboard_package}/bin/play_sounds.py --sample-dir /home/admin/samples";
        StandardInput = "socket";
        StandardOutput = "journal";
        StandardError = "journal";
      };
    };
  };
}
