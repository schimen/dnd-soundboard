"""
This python program is used to read input from stdin and play sounds depending
on respective commands. It is intended to receive commands from the
keyboard_controller.py program.
"""

from simpleaudio import WaveObject
from threading import Thread
from queue import Queue, Full
import os, re, sys
from commands import CommandEnum, receive_command

def open_sounds(path: str) -> dict[int, WaveObject]:
    sounds = {}
    if path is None:
        return sounds
    elif not os.path.isdir(path):
        return sounds

    for file in os.listdir(path):
        r = re.compile(r'(\d).*\.wav|mp3')
        m = r.match(file)
        if m:
            sample = m.group(1)
            sounds[int(sample)] = WaveObject.from_wave_file(os.path.join(path, file))

    return sounds

def play_sound_loop(sound: WaveObject, queue: Queue):
    playback = None
    while True:
        command = queue.get()
        if command == CommandEnum.PLAY_SOUND.value:
            if playback is not None:
                playback.stop()
            playback = sound.play()
        elif command == CommandEnum.STOP_SOUND.value:
            if playback is not None:
                playback.stop()
                playback = None
        elif command == CommandEnum.EXIT.value:
            if playback is not None:
                playback.stop()
            break

        queue.task_done()

def process_command(command: CommandEnum, args: tuple[str], message_queues: dict[int, Queue]) -> None:
    command_queues: list[Queue] = []
    
    if command == CommandEnum.PLAY_SOUND.value:
        if len(args) < 1:
            print('Play sound command got no arguments')
            return

        sound_number = int(args[0])
        command_queues.append(message_queues.get(sound_number))

    elif command == CommandEnum.STOP_SOUND.value:
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
    threads = {i: Thread(target=play_sound_loop, args=(sounds[i], queues[i])) for i in queues}
    for thread in threads.values():
        thread.start()
    return threads

def exit_threads(queues, threads):
    # Exit all threads
    for queue in queues.values():
        queue.put(CommandEnum.STOP_SOUND.value)
        queue.put(CommandEnum.EXIT.value)
    for thread in threads.values():
        thread.join()

def main():
    # Open each sound and create a queue and a thread for each of them
    bank_paths = {i: os.path.join(os.path.dirname(__file__), 'samples', f'bank_{i}') for i in range(1, 7)}
    sounds = open_sounds(bank_paths.get(1))
    queues = open_queues(sounds)
    threads = open_threads(sounds, queues)

    try:
        while True:
            command, args = receive_command()
            if command is CommandEnum.EXIT.value:
                break
            elif command == CommandEnum.NEW_BANK.value:
                if len(args) < 1:
                    print('Bank command got no arguments')
                    continue
                bank = int(args[0])
                exit_threads(queues, threads)
                sounds = open_sounds(bank_paths.get(bank))
                queues = open_queues(sounds)
                threads = open_threads(sounds, queues)

            elif command == CommandEnum.EXIT.value:
                break 

            else:
                process_command(command, args, queues)

    except KeyboardInterrupt:
            pass
    finally:
        exit_threads(queues, threads)
        return

if __name__ == '__main__':
    main()

