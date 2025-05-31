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
can be installed separately on a machine (see [Nix install page](https://nixos.org/download/)),
or a NixOS machine can be used.

Flakes are used for building the image, see [flakes on NixOS wiki](https://nixos.wiki/wiki/flakes)
for more information on how to enable and use this.

If the host computer compiling the image is not an arm machine, it is necessary to cross compile the image. The[NixOS_on_ARM](https://nixos.wiki/wiki/NixOS_on_ARM#Cross-compiling) entry on the NixOS wiki has a good explanation of how to enable cross compilation.

## Build and install the image

To build the image, run the following command:

```sh
nix build -L .#nixosConfigurations.zero2w.config.system.build.sdImage
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
nix run github:serokell/deploy-rs .#zero2w -- --ssh-user admin --hostname <device-ip>
```

## Access the device

mDNS is enabled on the device via the avahi daemon, and it should show up on
the local network by pinging `dnd-soundboard.local`. Another way to find the device
can be to connect the device to a monitor and run the command:

```sh
ifconfig wlan0
```

The local net can also be scanned for available units with the [arp-scan](https://github.com/royhills/arp-scan) program.

To access the device, SSH is enabled and it can be accessed with the admin user:

```sh
ssh admin@<device-ip>
```
