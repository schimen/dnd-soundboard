import sys
from enum import Enum

class CommandEnum(Enum):
    PLAY_SOUND='play'
    STOP_SOUND='stop'
    NEW_BANK='bank'
    EXIT='exit'

def send_command(command: CommandEnum, *args: str) -> None:
    message = str(command.value)
    for arg in args:
        message += ' ' + str(arg)
    sys.stdout.write(message + '\n')

def receive_command() -> tuple[CommandEnum, tuple[str]] | None:
    line = sys.stdin.readline()
    args = line.split()

    if len(args) == 0:
        return None, ()

    command = args[0]
    if command not in CommandEnum:
        return None, ()

    if len(args) > 1:
        return command, tuple(args[1:])
    else:
        return command, tuple()
