# Soundboard on NixOS

This directory contains files used for building a image file for Raspberry Pi
Zero 2 W with the soundboard software preinstalled and loaded.
[NixOS](https://nixos.org/) is used as operating system and Nix is used to
build the image.

The [nixos-pi-zero-2](https://github.com/plmercereau/nixos-pi-zero-2) repo by
plmercereau was of great help to get this working, and much of the
configuration here is copied from that repo.

## Configure network and access

To be able to access the machine without having to connect a screen or serial
console, wifi networks can be preconfigured and public SSH keys can be added
before building the image. Add the relevant wifi configuration and public key
to the [network-config.nix](./network-config.nix) file, and the
device should be accessible during the first boot.

## Build environment

Nix is used to build the image, and for this to work Nix must be installed. Nix
can be installed separately on a machine (see
[Nix install page](https://nixos.org/download/)), or a NixOS machine can be
used.

Flakes are used for building the image, see
[flakes on NixOS wiki](https://nixos.wiki/wiki/flakes) for more information on
how to enable and use this.

If the host computer compiling the image is not an arm machine, it is necessary
to cross compile the image. The
[NixOS_on_ARM](https://nixos.wiki/wiki/NixOS_on_ARM#Cross-compiling) entry on
the NixOS wiki has a good explanation of how to enable cross compilation.

The host environment is specified when building the image, and this document
assumes a `x86_64-linux` host platform. This must be changed when building
on a host machine with another architecture, a host machine running NixOS ARM64
architecture would use `aarch64-linux`.

### Development shell

To open a development shell for developing the C++ applications, run the
command:

```sh
nix develop
```

To open a development shell for the python test-application, run the command:

```sh
nix develop -L .#packages.x86_64-linux.soundboard_python
```


## Build and install the image

To cross compile the image from a x86 NixOS host machine, run the following
command:

```sh
nix build -L .#nixosConfigurations.x86_64-linux.zero2w.config.system.build.sdImage
```

To install the image on a disk, the following command can be used:

```sh
sudo dd if=result/sd-image/zero2.img of=/dev/<sd-card-disk> bs=1M conv=fsync status=progress
```

Alternatively, the [rpi-imager](https://github.com/raspberrypi/rpi-imager)
program can be used to flash the SD card. To do this, the `zero2.img` must be
chosen as a custom image.

To rebuild the system for a currently running image, the following command can
be used:

```sh
nix run github:serokell/deploy-rs .#zero2w -- --ssh-user soundplayer --hostname <device-ip>
```

## Access the device

mDNS is enabled on the device via the avahi daemon, and it should show up on
the local network by pinging `dnd-soundboard.local`. If this hostname shows up,
it can be used instead of the ip address when accessing the device.
Another way to find the device can be to connect the device to a monitor and
run the command:

```sh
ifconfig wlan0
```

The local net can also be scanned for available units with the
[arp-scan](https://github.com/royhills/arp-scan) program.

To access the device, SSH is enabled and it can be accessed with the
soundplayer user:

```sh
ssh soundplayer@<device-ip>
```

## Setting up

Must of the set-up of the device should be done before build, but it is
possible to manage network connections from the soundplayer user, and the
password to the Samba server must be set for it to be used.

To set the password of the samba user, run the command:

```sh
sudo smbpasswd -a soundplayer
```

To manage wireless networks, the [wpa_cli](https://linux.die.net/man/8/wpa_cli)
program can be used.

## Test image

A test image can be built and run in QEMU VM. The test image configuration is
defined in `testvm.nix`. To build this VM, run the command:

```sh
nix build -L .#nixosConfigurations.x86_64-linux.testvm.config.system.build.vm
```

The VM can be run using the `result/bin/run-dnd-soundboard-vm` script.

- The option `-nographic` can be added after the script to get shell access.
- The variable `QEMU_NET_OPTS=hostfwd=tcp::5555-:22` can be set before the
  command to gain access to SSH (over localhost, at port 5555).
- The options
  `-audiodev pipewire,id=audiodev1 -device ich9-intel-hda -device hda-duplex,audiodev=audiodev1`
  can be added to send sound to the host machine if it has PipeWire running.
