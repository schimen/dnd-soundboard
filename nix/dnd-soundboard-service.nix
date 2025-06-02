{ pkgs, ...}:
let
  soundboard_package = pkgs.callPackage ./dnd-soundboard.nix {};
  fifoFile = "/tmp/play_sounds.stdin";
in
{
  systemd.user = {
    sockets.play_sounds = {
      description = "Socket for sending commands to play_sounds.service";
      socketConfig = {
        ListenFIFO = fifoFile;
      };
    };
    services.play_sounds = {
      description = "Service that plays sounds when commands are received";
      serviceConfig = {
        Type = "simple";
        ExecStart = "${soundboard_package}/bin/play_sounds.py --sample-dir /home/admin/samples";
        StandardInput = "socket";
        StandardOutput = "journal";
        StandardError = "journal";
      };
    };
    services.keyboard_controller = {
      enable = true;
      serviceConfig = {
        Type = "simple";
        ExecStart = "${soundboard_package}/bin/keyboard_controller.py";
        StandardOutput = "file:${fifoFile}";
        StandardError = "journal";
      };
      wants = [ "play_sounds.socket" ];
      wantedBy = [ "default.target" ];
    };
  };
}
