# DnD Soundboard

The DnD soundboard is a small keyboard connected to a raspberry pi zero 2 w.
The raspberry pi responds to keypresses and play samples.

## Hardware

I found an old mechanical keyboard (a [Thura GXT 860](https://www.trust.com/en/product/21842))
in the electrical trash and that inspired this project. I cut the keyboard so
that only the numpad is left and removed some keys so that there was space for
the [Raspberry Pi Zero 2 W](https://www.raspberrypi.com/products/raspberry-pi-zero-2-w/)
that is used to recieve keypresses and play the sounds. The [raspiaudio MIC+](https://raspiaudio.com/product/mic/)
board provides a soundcard and speakers to the system.

## Software

The `python/keyboard_controller.py` program listens to keypresses from the
keyboard, and sends commands to STDOUT. This program should be run as root to
make sure it har access to the input events from the keyboard.

The `python/play_sounds.py` program listen to STDIN and play sounds
corresponding to the commands. SDL2 is used to make sound control efficient.

These programs should be run with and communicate through systemd services on
the raspberry pi. For testing, the following procedure can be used to run the
software:

- Create virtual serial files with [socat](https://linux.die.net/man/1/socat):
  `socat -d -d pty,rawer,echo=0 pty,rawer,echo=0`
- Read the names of the virtual files created from socat and run the following
  command from a new terminal:
  `sudo python3 python/keyboard_controller.py --device-name <device-name> > /dev/pts/X`
- Run the following command from another terminal:
  `cat /dev/pts/Y > python3 python/play_sounds.py`

There is a preconfigured NixOS image for Raspberry Pi Zero 2 W with the
soundboard software installed and set up. See [nix](./nix/README.md) for more
information on this.
