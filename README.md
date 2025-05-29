# DnD Soundboard

The DnD soundboard is a small keyboard connected to a raspberry pi zero 2 w.
The raspberry pi responds to keypresses and play samples.


## Software

The `keyboard_controller.py` program listens to keypresses from the keyboard,
and sends commands to STDOUT. This program should be run as root to make sure
it har access to the input events from the keyboard.

The `play_sounds.py` program listen to STDIN and play sounds corresponding to
the commands. SDL2 is used to make sound control efficient.

These programs should be run with and communicate through systemd services on
the raspberry pi. For testing, the following procedure can be used to run the
software:
- Create virtual serial files with [socat](https://linux.die.net/man/1/socat):
  `socat -d -d pty,rawer,echo=0 pty,rawer,echo=0`
- Read the names of the virtual files created from socat and run the following
  command from a new terminal:
  `sudo python3 keyboard_controller.py --device-name <device-name> > /dev/pts/X`
- Run the following command from another terminal:
  `cat /dev/pts/Y > python3 play_sounds.py`
