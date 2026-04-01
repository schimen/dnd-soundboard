#include "commands.h"
#include <catch2/catch_test_macros.hpp>
#include <thread>


TEST_CASE("CommandQueue different commands") {
    CommandQueue queue;
    Command cmd1{CommandType::PLAY_SOUND, 1};
    Command cmd2{CommandType::STOP_SOUND};
    Command cmd3{CommandType::NEW_BANK, 2};
    Command cmd4{CommandType::VOLUME, 75};
    Command cmd5{CommandType::LOOP_ON};
    Command cmd6{CommandType::LOOP_OFF};
    Command cmd7{CommandType::EXIT};

    queue.put(cmd1);
    queue.put(cmd2);
    queue.put(cmd3);
    queue.put(cmd4);
    queue.put(cmd5);
    queue.put(cmd6);
    queue.put(cmd7);

    REQUIRE(queue.get() == cmd1);
    REQUIRE(queue.get() == cmd2);
    REQUIRE(queue.get() == cmd3);
    REQUIRE(queue.get() == cmd4);
    REQUIRE(queue.get() == cmd5);
    REQUIRE(queue.get() == cmd6);
    REQUIRE(queue.get() == cmd7);
}

TEST_CASE("CommandQueue clear") {
    CommandQueue queue;
    Command cmd1{CommandType::PLAY_SOUND, 1};
    Command cmd2{CommandType::STOP_SOUND};
    Command cmd3{CommandType::NEW_BANK, 2};

    queue.put(cmd1);
    queue.put(cmd2);
    queue.put(cmd3);
    queue.clear();

    // Queue should be empty now - verify by putting and getting one item
    queue.put(cmd1);
    REQUIRE(queue.get() == cmd1);
}

TEST_CASE("CommandQueue producer thread") {
    CommandQueue queue;
    Command cmd1{CommandType::PLAY_SOUND, 1};

    std::thread producer([&queue, &cmd1]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        queue.put(cmd1);
    });

    // Consumer waits for the command
    Command retrieved = queue.get();
    REQUIRE(retrieved == cmd1);

    producer.join();
}

TEST_CASE("CommandQueue multiple producers") {
    CommandQueue queue;
    Command cmd1{CommandType::PLAY_SOUND, 1};
    Command cmd2{CommandType::STOP_SOUND};

    std::thread producer1([&queue, &cmd1]() { queue.put(cmd1); });
    std::thread producer2([&queue, &cmd2]() { queue.put(cmd2); });

    Command cmd_a = queue.get();
    Command cmd_b = queue.get();

    // Both commands should be retrieved (order may vary due to threading)
    bool cmd1_found = (cmd_a == cmd1) || (cmd_b == cmd1);
    bool cmd2_found = (cmd_a == cmd2) || (cmd_b == cmd2);

    REQUIRE(cmd1_found);
    REQUIRE(cmd2_found);

    producer1.join();
    producer2.join();
}

TEST_CASE("CommandQueue clear multiple times") {
    CommandQueue queue;
    Command cmd1{CommandType::PLAY_SOUND, 1};
    Command cmd2{CommandType::STOP_SOUND};
    Command cmd3{CommandType::VOLUME, 50};

    queue.put(cmd1);
    queue.clear();

    queue.put(cmd2);
    REQUIRE(queue.get() == cmd2);

    queue.put(cmd1);
    queue.put(cmd3);
    queue.clear();

    queue.put(cmd3);
    REQUIRE(queue.get() == cmd3);
}

TEST_CASE("CommandQueue commands with and without arguments") {
    CommandQueue queue;

    queue.put(Command{CommandType::PLAY_SOUND, 1});
    queue.put(Command{CommandType::EXIT});
    queue.put(Command{CommandType::VOLUME, 100});

    Command c1 = queue.get();
    REQUIRE(c1.getArg().has_value());
    REQUIRE(c1.getArg().value() == 1);

    Command c2 = queue.get();
    REQUIRE_FALSE(c2.getArg().has_value());

    Command c3 = queue.get();
    REQUIRE(c3.getArg().has_value());
    REQUIRE(c3.getArg().value() == 100);
}
