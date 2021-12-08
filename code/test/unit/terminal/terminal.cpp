#include <unity.h>
#include <Arduino.h>
#include <StreamString.h>

#include <terminal_commands.h>

// TODO: should we just use std::function at this point?
//       we don't actually benefit from having basic ptr functions in handler
//       test would be simplified too, we would no longer need to have static vars

// Got the idea from the Embedis test suite, set up a proxy for StreamString
// Real terminal processing happens with ringbuffer'ed stream
struct IOStreamString : public Stream {
    StreamString in;
    StreamString out;

    size_t write(uint8_t ch) final override {
        return in.write(ch);
    }

    int read() final override {
        return out.read();
    }

    int available() final override {
        return out.available();
    }

    int peek() final override {
        return out.peek();
    }

    void flush() final override {
        out.flush();
    }
};

// We need to make sure that our changes to split_args actually worked

void test_hex_codes() {

    static bool abc_done = false;

    terminal::Terminal::addCommand(F("abc"), [](::terminal::CommandContext&& ctx) {
        TEST_ASSERT_EQUAL(2, ctx.argv.size());
        TEST_ASSERT_EQUAL_STRING("abc", ctx.argv[0].c_str());
        TEST_ASSERT_EQUAL_STRING("abc", ctx.argv[1].c_str());
        abc_done = true;
    });

    IOStreamString str;
    str.out += String("abc \"\x61\x62\x63\"\r\n");

    terminal::Terminal handler(str);

    TEST_ASSERT_EQUAL(
        terminal::Terminal::Result::Command,
        handler.processLine()
    );
    TEST_ASSERT(abc_done);
}

// Ensure that we can register multiple commands (at least 3, might want to test much more in the future?)
// Ensure that registered commands can be called and they are called in order

void test_multiple_commands() {

    // set up counter to be chained between commands
    static int command_calls = 0;

    terminal::Terminal::addCommand(F("test1"), [](::terminal::CommandContext&& ctx) {
        TEST_ASSERT_EQUAL_MESSAGE(1, ctx.argv.size(), "Command without args should have argc == 1");
        TEST_ASSERT_EQUAL(0, command_calls);
        command_calls = 1;
    });
    terminal::Terminal::addCommand(F("test2"), [](::terminal::CommandContext&& ctx) {
        TEST_ASSERT_EQUAL_MESSAGE(1, ctx.argv.size(), "Command without args should have argc == 1");
        TEST_ASSERT_EQUAL(1, command_calls);
        command_calls = 2;
    });
    terminal::Terminal::addCommand(F("test3"), [](::terminal::CommandContext&& ctx) {
        TEST_ASSERT_EQUAL_MESSAGE(1, ctx.argv.size(), "Command without args should have argc == 1");
        TEST_ASSERT_EQUAL(2, command_calls);
        command_calls = 3;
    });

    IOStreamString str;
    str.out += String("test1\r\ntest2\r\ntest3\r\n");

    terminal::Terminal handler(str);

    // each processing step only executes a single command
    static int process_counter = 0;

    handler.process([](terminal::Terminal::Result result) -> bool {
        if (process_counter == 3) {
            TEST_ASSERT_EQUAL(result, terminal::Terminal::Result::NoInput);
            return false;
        } else {
            TEST_ASSERT_EQUAL(result, terminal::Terminal::Result::Command);
            ++process_counter;
            return true;
        }
        TEST_FAIL_MESSAGE("Should not be reached");
        return false;
    });
    TEST_ASSERT_EQUAL(3, command_calls);
    TEST_ASSERT_EQUAL(3, process_counter);

}

void test_command() {

    static int counter = 0;

    terminal::Terminal::addCommand(F("test.command"), [](::terminal::CommandContext&& ctx) {
        TEST_ASSERT_EQUAL_MESSAGE(1, ctx.argv.size(), "Command without args should have argc == 1");
        ++counter;
    });

    IOStreamString str;
    terminal::Terminal handler(str);

    TEST_ASSERT_EQUAL_MESSAGE(
        terminal::Terminal::Result::NoInput, handler.processLine(),
        "We have not read anything yet"
    );

    str.out += String("test.command\r\n");
    TEST_ASSERT_EQUAL(terminal::Terminal::Result::Command, handler.processLine());
    TEST_ASSERT_EQUAL_MESSAGE(1, counter, "At this time `test.command` was called just once");

    str.out += String("test.command");
    TEST_ASSERT_EQUAL(terminal::Terminal::Result::Pending, handler.processLine());
    TEST_ASSERT_EQUAL_MESSAGE(1, counter, "We are waiting for either \\r\\n or \\n, handler still has data buffered");

    str.out += String("\r\n");
    TEST_ASSERT_EQUAL(terminal::Terminal::Result::Command, handler.processLine());
    TEST_ASSERT_EQUAL_MESSAGE(2, counter, "We should call `test.command` the second time");

    str.out += String("test.command\n");
    TEST_ASSERT_EQUAL(terminal::Terminal::Result::Command, handler.processLine());
    TEST_ASSERT_EQUAL_MESSAGE(3, counter, "We should call `test.command` the third time, with just LF");

}

// Ensure that we can properly handle arguments

void test_command_args() {

    static bool waiting = false;

    terminal::Terminal::addCommand(F("test.command.arg1"), [](::terminal::CommandContext&& ctx) {
        TEST_ASSERT_EQUAL(2, ctx.argv.size());
        waiting = false;
    });

    terminal::Terminal::addCommand(F("test.command.arg1_empty"), [](::terminal::CommandContext&& ctx) {
        TEST_ASSERT_EQUAL(2, ctx.argv.size());
        TEST_ASSERT(!ctx.argv[1].length());
        waiting = false;
    });

    IOStreamString str;
    terminal::Terminal handler(str);

    waiting = true;
    str.out += String("test.command.arg1 test\r\n");
    TEST_ASSERT_EQUAL(terminal::Terminal::Result::Command, handler.processLine());
    TEST_ASSERT(!waiting);

    waiting = true;
    str.out += String("test.command.arg1_empty \"\"\r\n");
    TEST_ASSERT_EQUAL(terminal::Terminal::Result::Command, handler.processLine());
    TEST_ASSERT(!waiting);

}

// Ensure that we return error when nothing was handled, but we kept feeding the processLine() with data

void test_buffer() {

    IOStreamString str;
    str.out += String("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\r\n");

    terminal::Terminal handler(str, str.out.available() - 8);
    TEST_ASSERT_EQUAL(terminal::Terminal::Result::BufferOverflow, handler.processLine());

}

// sdssplitargs returns nullptr when quotes are not terminated and empty char for an empty string. we treat it all the same

void test_quotes() {

    terminal::Terminal::addCommand(F("test.quotes"), [](::terminal::CommandContext&& ctx) {
        for (auto& arg : ctx.argv) {
            TEST_MESSAGE(arg.c_str());
        }
        TEST_FAIL_MESSAGE("`test.quotes` should not be called");
    });

    IOStreamString str;
    terminal::Terminal handler(str);

    str.out += String("test.quotes \"quote without a pair\r\n");
    TEST_ASSERT_EQUAL(terminal::Terminal::Result::NoInput, handler.processLine());

    str.out += String("test.quotes 'quote without a pair\r\n");
    TEST_ASSERT_EQUAL(terminal::Terminal::Result::NoInput, handler.processLine());
    TEST_ASSERT_EQUAL(terminal::Terminal::Result::NoInput, handler.processLine());

}

// we specify that commands lowercase == UPPERCASE, both with hashed values and with equality functions
// (internal note: we use std::unordered_map at this time)

void test_case_insensitive() {

    terminal::Terminal::addCommand(F("test.lowercase1"), [](::terminal::CommandContext&& ctx) {
        TEST_FAIL_MESSAGE("`test.lowercase1` was registered first, but there's another function by the same name. This should not be called");
    });
    terminal::Terminal::addCommand(F("TEST.LOWERCASE1"), [](::terminal::CommandContext&& ctx) {
        __asm__ volatile ("nop");
    });

    IOStreamString str;
    terminal::Terminal handler(str);

    str.out += String("TeSt.lOwErCaSe1\r\n");
    TEST_ASSERT_EQUAL(terminal::Terminal::Result::Command, handler.processLine());

}

// We can use command ctx.output to send something back into the stream

void test_output() {

    terminal::Terminal::addCommand(F("test.output"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() != 2) return;
        ctx.output.print(ctx.argv[1]);
    });

    IOStreamString str;
    terminal::Terminal handler(str);

    char match[] = "test1234567890";
    str.out += String("test.output ") + String(match) + String("\r\n");
    TEST_ASSERT_EQUAL(terminal::Terminal::Result::Command, handler.processLine());
    TEST_ASSERT_EQUAL_STRING(match, str.in.c_str());

}

// When adding test functions, don't forget to add RUN_TEST(...) in the main()

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_command);
    RUN_TEST(test_command_args);
    RUN_TEST(test_multiple_commands);
    RUN_TEST(test_hex_codes);
    RUN_TEST(test_buffer);
    RUN_TEST(test_quotes);
    RUN_TEST(test_case_insensitive);
    RUN_TEST(test_output);
    UNITY_END();
}
