{ pkgs, python3Packages }:

python3Packages.buildPythonApplication rec {
    pname = "soundboard-python";
    version = "0.1.0";
    pyproject = false;

    propagatedBuildInputs = with python3Packages; [
        evdev
        dbus-python
        pysdl2
        pkgs.SDL2
    ];

    dontUnpack = true;
    installPhase = ''
        install -Dm755 "${../python/sound_player.py}" "$out/bin/sound_player.py"
        install -Dm755 "${../python/key_monitor.py}" "$out/bin/key_monitor.py"
        cp "${../python/commands.py}" "$out/bin/commands.py"
    '';
}
