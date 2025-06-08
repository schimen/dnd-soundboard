userName: sampleDir: { pkgs, ...}:
let
  soundboard_package = pkgs.callPackage ./dnd-soundboard.nix {};
  fifoFile = "/tmp/play_sounds.stdin";
in
{
  systemd.user.sockets = {
    play_sounds = {
      description = "Socket for sending commands to play_sounds.service";
      socketConfig = {
        ListenFIFO = fifoFile;
      };
    };
  };
  systemd.user.services = {
    play_sounds = {
      enable = true;
      description = "Service that plays sounds when commands are received";
      serviceConfig = {
        Type = "simple";
        ExecStartPre = "${pkgs.bash}/bin/bash -c 'mkdir -p -m 0777 ${sampleDir}'";
        ExecStart = "${soundboard_package}/bin/play_sounds.py --sample-dir ${sampleDir}";
        Restart = "always";
        RestartSec = "10";
        StandardInput = "socket";
        StandardOutput = "journal";
        StandardError = "journal";
      };
    };
    keyboard_controller = {
      enable = true;
      serviceConfig = {
        Type = "simple";
        ExecStart = "${soundboard_package}/bin/keyboard_controller.py";
        Restart = "always";
        RestartSec = "10";
        StandardOutput = "file:${fifoFile}";
        StandardError = "journal";
      };
      wants = [ "play_sounds.socket" ];
      wantedBy = [ "default.target" ];
    };
  };
}
