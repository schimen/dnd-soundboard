#!/usr/bin/env python3
"""
This python program is used to read input event from the soundboard controller
and send the respective commands to stdout. This is intended to be run as root,
and the stdout piped to the stdin of the play_sounds.py program.
"""

import sys
import traceback
from time import time, sleep
from argparse import ArgumentParser, Namespace
from evdev import InputDevice, InputEvent, ecodes, list_devices
from commands import CommandEnum, send_command

# Keys that represent each sound
SOUND_KEYS = {69: 1, 98: 2, 55: 3, 71: 4, 72: 5, 73: 6}
# Keys that represent the different sound banks
BANK_KEYS = {69: 1, 98: 2, 71: 3, 72: 4}
# Volume keys and steps
VOLUME_KEYS = {55: +5, 73: -5}
# When this key is held, the alternative function for a key is used
FN_KEY = 74
# This button sends stop signal
STOP_KEY = 78
# When this button is held for 5 seconds, the system shuts down
SHUTDOWN_HOLD_TIME = 3

def process_keys(device: InputDevice, event: InputEvent) -> bool:
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
    
    # Volume change
    elif event.code in VOLUME_KEYS and FN_KEY in device.active_keys():
        new_leds[ecodes.LED_NUML] = 1 if is_active(event) else 0
        volume_diff = VOLUME_KEYS[event.code]
        if is_press(event):
            process_keys.volume += volume_diff
            if process_keys.volume > 100:
                process_keys.volume = 100
            elif process_keys.volume < 0:
                process_keys.volume = 0

            send_command(CommandEnum.VOLUME, process_keys.volume)

    # Bank event
    elif event.code in BANK_KEYS and FN_KEY in device.active_keys():
        new_leds[ecodes.LED_NUML] = 1 if is_active(event) else 0
        if is_press(event):
            bank_number = BANK_KEYS[event.code]
            send_command(CommandEnum.NEW_BANK, bank_number)

            # Set loop mode for new bank
            if process_keys.release_mode:
                send_command(CommandEnum.LOOP_ON)
            else:
                send_command(CommandEnum.LOOP_OFF)

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
                return True

    # Change release mode
    elif event.code == STOP_KEY and FN_KEY in device.active_keys():
        if is_press(event):
            process_keys.release_mode = not process_keys.release_mode
            if process_keys.release_mode:
                new_leds[ecodes.LED_CAPSL] = 1
                send_command(CommandEnum.LOOP_ON)
            else:
                new_leds[ecodes.LED_CAPSL] = 0
                send_command(CommandEnum.LOOP_OFF)

    for led, value in new_leds.items():
        device.set_led(led, value)

    process_keys.last_leds = device.leds()

    # We will not exit yet
    return False

process_keys.last_leds = []

process_keys.last_shutdown_press = time()
process_keys.shutdown_last_hold = time()
process_keys.release_mode = False
process_keys.volume = 50

def is_active(event: InputEvent) -> bool:
    return is_press(event) or is_hold(event)

def is_hold(event: InputEvent) -> bool:
    return event.value == 2

def is_press(event: InputEvent) -> bool:
    return event.value == 1

def is_release(event: InputEvent) -> bool:
    return event.value == 0

def get_device_by_name(name: str) -> InputDevice:
    devices = [InputDevice(path) for path in list_devices()]
    for device in devices:
        if name in device.name:
            return device

def cycle_leds(device: InputDevice) -> None:
    leds = (ecodes.LED_NUML, ecodes.LED_CAPSL, ecodes.LED_SCROLLL)
    for led in leds:
        device.set_led(led, 1)
        sleep(0.1)
    for led in leds:
        device.set_led(led, 0)
        sleep(0.1)

def read_keys(device: InputDevice) -> None:
    # Set initial volume before starting
    send_command(CommandEnum.VOLUME, process_keys.volume)

    # Read events from device and send commands for every event until exit
    # command
    for event in device.read_loop():
        if event.type == ecodes.EV_KEY:
            exit_status = process_keys(device, event)
            if (exit_status):
                break

def main(args: Namespace) -> None:
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
    parser = ArgumentParser(
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
        default='SEMICO GXT 860 Keyboard',
        help='Part of the name of the input event file, will use first file' \
             'with name matching argument. This option is ignored if' \
             'device-file is set.'
    )
    args = parser.parse_args()

    try:
        main(args)
    except KeyboardInterrupt:
        sys.stderr.write('Terminated program with keyboard interrupt')
    except Exception:
        sys.stderr.write(traceback.format_exc() + '\n')
