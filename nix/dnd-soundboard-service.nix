userName: sampleDir: { pkgs, ...}:
let
  soundboard_package = pkgs.callPackage ./dnd-soundboard.nix {};
  fifoFile = "/tmp/sound_player.stdin";
in
{
  systemd.user.sockets = {
    sound_player = {
      description = "Socket for sending commands to sound_player.service";
      socketConfig = {
        ListenFIFO = fifoFile;
      };
    };
  };
  systemd.user.services = {
    sound_player = {
      enable = true;
      description = "Service that plays sounds when commands are received";
      serviceConfig = {
        Type = "simple";
        ExecStartPre = "${pkgs.bash}/bin/bash -c 'mkdir -p -m 0755 ${sampleDir}'";
        ExecStart = "${soundboard_package}/bin/sound_player.py --sample-dir ${sampleDir}";
        Restart = "always";
        RestartSec = "10";
        StandardInput = "socket";
        StandardOutput = "journal";
        StandardError = "journal";
      };
    };
    key_monitor = {
      enable = true;
      serviceConfig = {
        Type = "simple";
        ExecStart = "${soundboard_package}/bin/key_monitor.py";
        Restart = "always";
        RestartSec = "10";
        StandardOutput = "file:${fifoFile}";
        StandardError = "journal";
      };
      wants = [ "sound_player.socket" ];
      wantedBy = [ "default.target" ];
    };
  };
}
