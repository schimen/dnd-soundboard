#!/usr/bin/env python3
"""
This python program is used to read input from stdin and play sounds depending
on respective commands. It is intended to receive commands from the
keyboard_controller.py program.
"""

import os
import re
import sys
from threading import Thread
from queue import Queue, Full
from sdl2.sdlmixer import Mix_Init, Mix_Quit, Mix_OpenAudio, Mix_CloseAudio, \
    Mix_LoadWAV, Mix_Chunk, Mix_FreeChunk, Mix_AllocateChannels, \
    Mix_PlayChannel, Mix_Pause, Mix_HaltChannel, MIX_INIT_MP3, \
    MIX_DEFAULT_FORMAT, Mix_GetError
from commands import CommandEnum, receive_command


def open_sounds(path: str) -> dict[int, Mix_Chunk]:
    sounds = {}
    if path is None:
        Mix_AllocateChannels(0)
        return sounds
    elif not os.path.isdir(path):
        Mix_AllocateChannels(0)
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
    while True:
        command = queue.get()
        if command == CommandEnum.PLAY_SOUND:
            if not sound:
                sys.stderr.write(
                    f'Sound {sound} on channel {channel}is not available\n')
                continue

            res = Mix_PlayChannel(channel, sound, 0)
            if res < 0:
                sys.stderr.write(
                    f'Could not play sound at channel {channel}: ' +
                    f'{Mix_GetError().decode()}\n')

        elif command == CommandEnum.STOP_SOUND:
            Mix_Pause(channel)
        elif command == CommandEnum.EXIT:
            Mix_HaltChannel(channel)
            break

        queue.task_done()

    Mix_FreeChunk(sound)


def process_command(
        command: CommandEnum, args: tuple[str],
        message_queues: dict[int, Queue]) -> None:
    command_queues: list[Queue] = []

    if command == CommandEnum.PLAY_SOUND:
        if len(args) < 1:
            sys.stderr.write('Play sound command got no arguments\n')
            return

        sound_number = int(args[0])
        command_queues.append(message_queues.get(sound_number))

    elif command == CommandEnum.STOP_SOUND:
        if len(args) > 0:
            # Only stop a single sound
            sound_number = int(args[0])
            command_queues.append(message_queues.get(sound_number))
        else:
            # Stop all sounds
            for queue in message_queues.values():
                command_queues.append(queue)

    for queue in command_queues:
        if queue is None or command is None:
            continue
        try:
            queue.put_nowait(command)
        except Full:
            pass

    return command


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


def main():
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
    bank_paths = {i: os.path.join(os.path.dirname(
        __file__), 'samples', f'bank_{i}') for i in range(1, 7)}
    sounds = open_sounds(bank_paths.get(1))
    queues = open_queues(sounds)
    threads = open_threads(sounds, queues)

    try:
        while True:
            command, args = receive_command()
            if command is None:
                continue

            if command is CommandEnum.EXIT:
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
    finally:
        exit_threads(queues, threads)
        Mix_CloseAudio()
        Mix_Quit()
        return

if __name__ == '__main__':
    main()
