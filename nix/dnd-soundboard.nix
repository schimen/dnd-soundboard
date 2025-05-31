{ pkgs, lib, python3Packages }:

python3Packages.buildPythonApplication rec {
    pname = "dnd-soundboard";
    version = "0.1.0";
    pyproject = false;

    propagatedBuildInputs = with python3Packages; [
        evdev
        pysdl2
        pkgs.SDL2
    ];

    dontUnpack = true;
    installPhase = ''
        install -Dm755 "${../python/play_sounds.py}" "$out/bin/play_sounds.py"
        install -Dm755 "${../python/keyboard_controller.py}" "$out/bin/keyboard_controller.py"
        cp "${../python/commands.py}" "$out/bin/commands.py"
    '';
}