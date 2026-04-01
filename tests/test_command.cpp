#include "commands.h"
#include <catch2/catch_test_macros.hpp>
#include <sstream>

TEST_CASE("Command construction with argument") {
    Command playSoundCmd{CommandType::PLAY_SOUND, 1};
    REQUIRE(playSoundCmd.getType() == CommandType::PLAY_SOUND);
    REQUIRE(playSoundCmd.getArg().has_value());
    REQUIRE(playSoundCmd.getArg().value() == 1);
}

TEST_CASE("Command construction without argument") {
    Command exitCmd{CommandType::EXIT};
    REQUIRE(exitCmd.getType() == CommandType::EXIT);
    REQUIRE_FALSE(exitCmd.getArg().has_value());
}

TEST_CASE("Command string conversion") {
    Command playSoundCmd{CommandType::PLAY_SOUND, 1};
    Command stopSoundCmd{CommandType::STOP_SOUND, 2};
    Command stopAllCmd{CommandType::STOP_SOUND};
    Command newBankCmd{CommandType::NEW_BANK, 3};
    Command volumeCmd{CommandType::VOLUME, 50};
    Command loopOnCmd{CommandType::LOOP_ON};
    Command loopOffCmd{CommandType::LOOP_OFF};
    Command exitCmd{CommandType::EXIT};

    REQUIRE(std::string(playSoundCmd) == "play 1");
    REQUIRE(std::string(stopSoundCmd) == "stop 2");
    REQUIRE(std::string(stopAllCmd) == "stop");
    REQUIRE(std::string(newBankCmd) == "bank 3");
    REQUIRE(std::string(volumeCmd) == "volume 50");
    REQUIRE(std::string(loopOnCmd) == "loop");
    REQUIRE(std::string(loopOffCmd) == "oneshot");
    REQUIRE(std::string(exitCmd) == "exit");
}

TEST_CASE("Command stream output operator") {
    Command playSoundCmd{CommandType::PLAY_SOUND, 1};
    Command exitCmd{CommandType::EXIT};

    SECTION("play sound command") {
        std::stringstream ss;
        ss << playSoundCmd;
        REQUIRE(ss.str() == "play 1");
    }

    SECTION("exit command") {
        std::stringstream ss;
        ss << exitCmd;
        REQUIRE(ss.str() == "exit");
    }
}

TEST_CASE("Command equality operator") {
    Command cmd1{CommandType::PLAY_SOUND, 1};
    Command cmd2{CommandType::PLAY_SOUND, 1};
    Command cmd3{CommandType::PLAY_SOUND, 2};
    Command cmd4{CommandType::STOP_SOUND, 1};

    REQUIRE(cmd1 == cmd2);
    REQUIRE_FALSE(cmd1 == cmd3);
    REQUIRE_FALSE(cmd1 == cmd4);
}

TEST_CASE("Command equality without arguments") {
    Command cmd1{CommandType::EXIT};
    Command cmd2{CommandType::EXIT};
    Command cmd3{CommandType::LOOP_ON};

    REQUIRE(cmd1 == cmd2);
    REQUIRE_FALSE(cmd1 == cmd3);
}

TEST_CASE("Command type conversion") {
    Command playSoundCmd{CommandType::PLAY_SOUND, 1};
    Command exitCmd{CommandType::EXIT};

    REQUIRE(static_cast<CommandType>(playSoundCmd) == CommandType::PLAY_SOUND);
    REQUIRE(static_cast<CommandType>(exitCmd) == CommandType::EXIT);
}

TEST_CASE("Command all command types") {
    Command playSoundCmd{CommandType::PLAY_SOUND, 1};
    Command stopSoundCmd{CommandType::STOP_SOUND, 2};
    Command newBankCmd{CommandType::NEW_BANK, 3};
    Command volumeCmd{CommandType::VOLUME, 50};
    Command loopOnCmd{CommandType::LOOP_ON};
    Command loopOffCmd{CommandType::LOOP_OFF};
    Command exitCmd{CommandType::EXIT};

    REQUIRE(playSoundCmd.getType() == CommandType::PLAY_SOUND);
    REQUIRE(stopSoundCmd.getType() == CommandType::STOP_SOUND);
    REQUIRE(newBankCmd.getType() == CommandType::NEW_BANK);
    REQUIRE(volumeCmd.getType() == CommandType::VOLUME);
    REQUIRE(loopOnCmd.getType() == CommandType::LOOP_ON);
    REQUIRE(loopOffCmd.getType() == CommandType::LOOP_OFF);
    REQUIRE(exitCmd.getType() == CommandType::EXIT);
}

TEST_CASE("Command optional arguments") {
    Command withArg{CommandType::VOLUME, 75};
    Command withoutArg{CommandType::LOOP_ON};

    REQUIRE(withArg.getArg().has_value());
    REQUIRE_FALSE(withoutArg.getArg().has_value());
}

TEST_CASE("Command argument edge cases") {
    Command zeroArg{CommandType::VOLUME, 0};
    Command negativeArg{CommandType::VOLUME, -10};
    Command largeArg{CommandType::VOLUME, 1000};

    REQUIRE(zeroArg.getArg().value() == 0);
    REQUIRE(negativeArg.getArg().value() == -10);
    REQUIRE(largeArg.getArg().value() == 1000);
}
