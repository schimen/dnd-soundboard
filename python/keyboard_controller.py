#!/usr/bin/env python3
"""
This python program is used to read input event from the soundboard controller
and send the respective commands to stdout. This is intended to be run as root,
and the stdout piped to the stdin of the play_sounds.py program.
"""

import argparse
import sys
from time import time, sleep
from evdev import InputDevice, ecodes, list_devices
from commands import CommandEnum, send_command

# Keys that represent each sound
SOUND_KEYS = {69: 1, 98: 2, 55: 3, 71: 4, 72: 5, 73: 6}
# Keys that represent the different sound banks
BANK_KEYS = SOUND_KEYS
# When this key is held, the alternative function for a key is used
FN_KEY = 74
# This button sends stop signal
STOP_KEY = 78
# When this button is held for 5 seconds, the system shuts down
SHUTDOWN_HOLD_TIME = 3

def process_keys(device, event):
    # Leds to set at the end of function, led number is key and led status is
    # value
    new_leds = {}

    # Numlock key resets the leds, manually set leds to previous position
    if event.code == ecodes.KEY_NUMLOCK:
        for led in process_keys.last_leds:
            new_leds[led] = 1

    # Light up function led
    if event.code == FN_KEY:
        new_leds[ecodes.LED_SCROLLL] = 1 if is_active(event) else 0

    # Sound event
    if event.code in SOUND_KEYS and FN_KEY not in device.active_keys():
        sound_number = SOUND_KEYS[event.code]
        new_leds[ecodes.LED_NUML] = 1 if is_active(event) else 0
        if is_press(event):
            send_command(CommandEnum.PLAY_SOUND, sound_number)
        if is_release(event) and process_keys.release_mode:
            send_command(CommandEnum.STOP_SOUND, sound_number)

    # Bank event
    elif event.code in BANK_KEYS and FN_KEY in device.active_keys():
        new_leds[ecodes.LED_NUML] = 1 if is_active(event) else 0
        if is_press(event):
            bank_number = BANK_KEYS[event.code]
            send_command(CommandEnum.NEW_BANK, bank_number)

    # Stop event and shutdown event if hold for a longer time
    elif event.code == STOP_KEY and FN_KEY not in device.active_keys():
        new_leds[ecodes.LED_NUML] = 1 if is_active(event) else 0
        if is_press(event):
            process_keys.last_shutdown_press = time()
            send_command(CommandEnum.STOP_SOUND)

        elif is_release(event):
            process_keys.last_shutdown_press = None

        elif is_hold(event) and process_keys.last_shutdown_press is not None:
            hold_duration = time() - process_keys.last_shutdown_press
            if hold_duration > SHUTDOWN_HOLD_TIME:
                send_command(CommandEnum.EXIT)
                process_keys.last_shutdown_press = None
                cycle_leds(device)
                sys.exit()

    # Change release mode
    elif event.code == STOP_KEY and FN_KEY in device.active_keys():
        if is_press(event):
            process_keys.release_mode = not process_keys.release_mode
            new_leds[ecodes.LED_CAPSL] = 1 if process_keys.release_mode else 0

    for led, value in new_leds.items():
        device.set_led(led, value)

    process_keys.last_leds = device.leds()

process_keys.last_leds = []

process_keys.last_shutdown_press = time()
process_keys.shutdown_last_hold = time()
process_keys.release_mode = False

def is_active(event):
    return is_press(event) or is_hold(event)

def is_hold(event):
    return event.value == 2

def is_press(event):
    return event.value == 1

def is_release(event):
    return event.value == 0

def get_device_by_name(name):
    devices = [InputDevice(path) for path in list_devices()]
    for device in devices:
        if name in device.name:
            return device

def cycle_leds(device):
    leds = (ecodes.LED_NUML, ecodes.LED_CAPSL, ecodes.LED_SCROLLL)
    for led in leds:
        device.set_led(led, 1)
        sleep(0.1)
    for led in leds:
        device.set_led(led, 0)
        sleep(0.1)

def read_keys(device):
    for event in device.read_loop():
        if event.type == ecodes.EV_KEY:
            process_keys(device, event)

def main(args):
    if args.device_file is not None:
        device = InputDevice(args.device_file)
    else:
        device = get_device_by_name(args.device_name)

    if device is None:
        sys.stderr.write('Could not find any matching devices\n')
        return
    
    sys.stderr.write(f'Open event file {device.path}: {device.name}\n')
    cycle_leds(device)
    
    try:
        read_keys(device)
    except OSError as e:
        sys.stderr.write(f'Caught OS exception: {e}\n')
    finally:
        device.close()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='controller_manager',
        description='Program used to read and control soundboard controller'
    )
    parser.add_argument(
        '--device-file',
        default=None,
        help='Path to input event file for button device'
    )
    parser.add_argument(
        '--device-name',
        default='GXT',
        help='Part of the name of the input event file, will use first file' \
             'with name matching argument. This option is ignored if' \
             'device-file is set.'
    )
    args = parser.parse_args()

    try:
        main(args)
    except KeyboardInterrupt:
        sys.stderr.write('Terminated program with keyboard interrupt')
