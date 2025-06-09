#!/usr/bin/env python3
"""
This python program is used to read input from stdin and play sounds depending
on respective commands. It is intended to receive commands from the
keyboard_controller.py program.
"""

import os
import re
import sys
import traceback
from threading import Thread
from queue import Queue, Full
from argparse import ArgumentParser
import dbus
from sdl2.sdlmixer import Mix_Init, Mix_Quit, Mix_OpenAudio, Mix_CloseAudio, \
    Mix_LoadWAV, Mix_Chunk, Mix_FreeChunk, Mix_AllocateChannels, \
    Mix_PlayChannel, Mix_Pause, Mix_HaltChannel, Mix_MasterVolume, \
    Mix_GetError, MIX_INIT_MP3, MIX_DEFAULT_FORMAT, MIX_MAX_VOLUME
from commands import CommandEnum, receive_command


def shut_down_system() -> None:
    sys_bus = dbus.SystemBus()
    ck_srv = sys_bus.get_object('org.freedesktop.login1',
                                '/org/freedesktop/login1')
    ck_iface = dbus.Interface(ck_srv, 'org.freedesktop.login1.Manager')
    if not ck_iface.get_dbus_method('CanPowerOff')():
        sys.stderr.write('Error: User can not use DBus power-off command\n')
        return

    ck_iface.get_dbus_method('PowerOff')(False)


def open_sounds(path: str) -> dict[int, Mix_Chunk]:
    sounds = {}
    if path is None:
        Mix_AllocateChannels(0)
        sys.stderr.write(f'Got no directory\n')
        return sounds
    elif not os.path.isdir(path):
        Mix_AllocateChannels(0)
        sys.stderr.write(f'Path {path} is not a directory\n')
        return sounds

    for file in os.listdir(path):
        filepath = os.path.join(path, file)
        r = re.compile(r'(\d).*\.wav|mp3')
        m = r.match(file)
        if m:
            sample = m.group(1)
            chunk = Mix_LoadWAV(filepath.encode())
            if not chunk:
                sys.stderr.write(f'Could not read audio file {file}\n')
                continue

            sounds[int(sample)] = chunk

    Mix_AllocateChannels(len(sounds))
    if Mix_AllocateChannels(-1) != len(sounds):
        sys.stderr.write(f'Could not open channels\n')
    return sounds


def play_sound_loop(channel: int, sound: Mix_Chunk, queue: Queue):
    loop_state = 0
    while True:
        command = queue.get()
        if command == CommandEnum.PLAY_SOUND:
            if not sound:
                sys.stderr.write(
                    f'Sound {sound} on channel {channel}is not available\n')
                continue

            res = Mix_PlayChannel(channel, sound, loop_state)
            if res < 0:
                sys.stderr.write(
                    f'Could not play sound at channel {channel}: ' +
                    f'{Mix_GetError().decode()}\n')

        elif command == CommandEnum.STOP_SOUND:
            Mix_Pause(channel)

        elif command == CommandEnum.LOOP_ON:
            loop_state = -1

        elif command == CommandEnum.LOOP_OFF:
            loop_state = 0

        elif command == CommandEnum.EXIT:
            Mix_HaltChannel(channel)
            break

        queue.task_done()

    Mix_FreeChunk(sound)


def process_command(
        command: CommandEnum, args: tuple[str],
        message_queues: dict[int, Queue]) -> None:
    command_queues: list[Queue] = []

    # If there is an argument, make sure it is an integer
    arg_value = None
    if len(args) > 0:
        try:
            arg_value = int(args[0])
        except ValueError as e:
            sys.stderr.write(f'Error {e}: Could not convert argument ' + \
                             f'{args[0]} to int\n')
            return

    if command == CommandEnum.PLAY_SOUND:
        if arg_value is None:
            sys.stderr.write('Play sound command got no valid arguments\n')
        else:
            command_queues.append(message_queues.get(arg_value))

    elif command == CommandEnum.STOP_SOUND:
        if arg_value is None:
            # Stop all sounds
            for queue in message_queues.values():
                command_queues.append(queue)
        else:
            # Only stop a single sound
            command_queues.append(message_queues.get(arg_value))

    elif command == CommandEnum.VOLUME:
        if arg_value is None:
            sys.stderr.write('Volume command got no valid arguments\n')
        else:
            volume = int(round(arg_value * (MIX_MAX_VOLUME/100)))
            Mix_MasterVolume(volume)

    elif command in (CommandEnum.LOOP_ON, CommandEnum.LOOP_OFF):
        # Set new loop state in all queues
        for queue in message_queues.values():
            command_queues.append(queue)

    for queue in command_queues:
        if queue is None or command is None:
            continue
        try:
            queue.put_nowait(command)
        except Full:
            pass

def open_queues(sounds):
    return {i: Queue(10) for i in sounds}


def open_threads(sounds, queues):
    threads = {i: Thread(target=play_sound_loop, args=(
        i, sounds[sample_id], queues[sample_id])) for
        i, sample_id in enumerate(queues)}
    for thread in threads.values():
        thread.start()
    return threads


def exit_threads(queues, threads):
    # Exit all threads
    for queue in queues.values():
        queue.put(CommandEnum.STOP_SOUND)
        queue.put(CommandEnum.EXIT)
    for thread in threads.values():
        thread.join()


def main(args):
    # Open sound interface
    res = Mix_Init(MIX_INIT_MP3)
    if res != MIX_INIT_MP3:
        sys.stderr.write(
            f'Could not initialize mixer: {Mix_GetError().decode()}\n')
        return
    res = Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024)
    if res < 0:
        sys.stderr.write(
            f'Could not open audio device: {Mix_GetError().decode()}\n')
        return

    # Open each sound and create a queue and a thread for each of them
    bank_paths = {i: os.path.join(args.sample_dir, f'bank_{i}') for i in range(1, 7)}
    sounds = open_sounds(bank_paths.get(1))
    queues = open_queues(sounds)
    threads = open_threads(sounds, queues)

    try:
        while True:
            command, args = receive_command()
            if command is None:
                continue

            if command is CommandEnum.EXIT:
                shut_down_system()
                break

            elif command == CommandEnum.NEW_BANK:
                if len(args) < 1:
                    sys.stderr.write('Bank command got no arguments\n')
                    continue
                bank = int(args[0])
                exit_threads(queues, threads)
                sounds = open_sounds(bank_paths.get(bank))
                queues = open_queues(sounds)
                threads = open_threads(sounds, queues)

            else:
                process_command(command, args, queues)

    except KeyboardInterrupt:
        pass
    except Exception:
        sys.stderr.write(traceback.format_exc() + '\n')
    finally:
        exit_threads(queues, threads)
        Mix_CloseAudio()
        Mix_Quit()
        return

if __name__ == '__main__':
    parser = ArgumentParser(
        prog='play_sounds',
        description='Parse incoming commands and play the corresponding sounds'
    )
    parser.add_argument(
        '--sample-dir',
        default='samples',
        help='Directory with samples'
    )
    args = parser.parse_args()
    main(args)
