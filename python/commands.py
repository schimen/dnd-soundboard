import sys
from enum import StrEnum

class CommandEnum(StrEnum):
    PLAY_SOUND='play'
    STOP_SOUND='stop'
    NEW_BANK='bank'
    VOLUME='volume'
    LOOP_ON='loop'
    LOOP_OFF='oneshot'
    EXIT='exit'

def send_command(command: CommandEnum, *args: str) -> None:
    message = str(command.value)
    for arg in args:
        message += ' ' + str(arg)
    sys.stdout.write(message + '\n')
    sys.stdout.flush()

def receive_command() -> tuple[CommandEnum, tuple[str]] | None:
    line = sys.stdin.readline().rstrip()
    args = line.split()

    if len(args) == 0:
        return None, ()

    command_str = args[0]
    if command_str not in CommandEnum:
        return None, ()

    command = CommandEnum(command_str)
    if len(args) > 1:
        return command, tuple(args[1:])
    else:
        return command, tuple()

if __name__ == '__main__':
    # Run some commands just to test
    from time import sleep
    test_commands = (
        (CommandEnum.NEW_BANK, 1),
        (CommandEnum.PLAY_SOUND, 1),
        (CommandEnum.PLAY_SOUND, 2),
        (CommandEnum.STOP_SOUND,),
        (CommandEnum.NEW_BANK, 2),
        (CommandEnum.PLAY_SOUND, 1),
        (CommandEnum.PLAY_SOUND, 2),
        (CommandEnum.EXIT,),
    )
    try:
        for command in test_commands:
            send_command(*command)
            sleep(1)
    except KeyboardInterrupt:
        pass
