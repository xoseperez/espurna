#include <unity.h>
#include <Arduino.h>
#include <StreamString.h>

#include <espurna/libs/PrintString.h>
#include <espurna/terminal_commands.h>

namespace espurna {
namespace terminal {
namespace test {
namespace {

// Default 'print nothing' case
struct BlackHole : public Print {
    size_t write(uint8_t) override {
        return 0;
    }

    size_t write(const uint8_t*, size_t) override {
        return 0;
    }
};

BlackHole DefaultOutput;

// We need to make sure that our changes to split_args actually worked
void test_hex_codes() {
    {
        const char input[] = "abc \"\\x";
        const auto result = parse_line(input);
        TEST_ASSERT_EQUAL_STRING("InvalidEscape", parser::error(result.error).c_str());
    }

    {
        const char input[] = "abc \"\\x5";
        const auto result = parse_line(input);
        TEST_ASSERT_EQUAL_STRING("InvalidEscape", parser::error(result.error).c_str());
    }

    {
        static bool abc_done = false;

        add("abc", [](CommandContext&& ctx) {
            TEST_ASSERT_EQUAL(2, ctx.argv.size());
            TEST_ASSERT_EQUAL_STRING("abc", ctx.argv[0].c_str());
            TEST_ASSERT_EQUAL_STRING("abc", ctx.argv[1].c_str());
            abc_done = true;
        });

        const char input[] = "abc \"\\x61\\x62\\x63\"\r\n";

        const auto result = parse_line(input);
        TEST_ASSERT_EQUAL(2, result.argv.size());
        TEST_ASSERT_EQUAL_STRING("Ok", parser::error(result.error).c_str());
        TEST_ASSERT_EQUAL_STRING("abc", result.argv[0].c_str());
        TEST_ASSERT_EQUAL_STRING("abc", result.argv[1].c_str());

        TEST_ASSERT(find_and_call(result, DefaultOutput));
        TEST_ASSERT(abc_done);
    }
}

// Ensure parsing function does not cause nearby strings to be included
void test_parse_overlap() {
    const char input[] =
        "three\r\n"
        "two\r\n"
        "one\r\n";

    const auto* ptr = &input[0];
    const auto* end = &input[__builtin_strlen(input)];

    {
        const auto eol = std::find(ptr, end, '\n');
        const auto result = parse_line(StringView(ptr, std::next(eol)));
        TEST_ASSERT_EQUAL(parser::Error::Ok, result.error);
        TEST_ASSERT(std::next(eol) != end);
        ptr = std::next(eol);

        TEST_ASSERT_EQUAL(1, result.argv.size());
        TEST_ASSERT_EQUAL_STRING("three", result.argv[0].c_str());
    }

    {
        const auto eol = std::find(ptr, end, '\n');
        const auto result = parse_line(StringView(ptr, std::next(eol)));
        TEST_ASSERT_EQUAL(parser::Error::Ok, result.error);
        TEST_ASSERT(std::next(eol) != end);
        ptr = std::next(eol);

        TEST_ASSERT_EQUAL(1, result.argv.size());
        TEST_ASSERT_EQUAL_STRING("two", result.argv[0].c_str());
    }

    {
        const auto eol = std::find(ptr, end, '\n');
        const auto result = parse_line(StringView(ptr, std::next(eol)));
        TEST_ASSERT_EQUAL(parser::Error::Ok, result.error);
        TEST_ASSERT(std::next(eol) == end);
        TEST_ASSERT_EQUAL(1, result.argv.size());
        TEST_ASSERT_EQUAL_STRING("one", result.argv[0].c_str());
    }
}

// recent terminal version also allows a static commands list instead of passing
// each individual name+func one by one
void test_commands_array() {
    static int command_calls = 0;

    static Command commands[] {
        Command{.name = "array.one", .func = [](CommandContext&&) {
            ++command_calls;
        }},
        Command{.name = "array.two", .func = [](CommandContext&&) {
            ++command_calls;
        }},
        Command{.name = "array.three", .func = [](CommandContext&&) {
            ++command_calls;
        }},
    };

    const auto before = size();
    add(Commands{std::begin(commands), std::end(commands)});

    TEST_ASSERT_EQUAL(before + 3, size());

    const char input[] = "array.one\narray.two\narray.three\n";
    TEST_ASSERT(api_find_and_call(input, DefaultOutput));
    TEST_ASSERT_EQUAL(3, command_calls);
}

// Ensure that we can register multiple commands (at least 3, might want to test much more in the future?)
// Ensure that registered commands can be called and they are called in order
void test_multiple_commands() {
    // make sure commands execute in sequence
    static int command_calls = 0;

    add("test1", [](CommandContext&& ctx) {
        TEST_ASSERT_EQUAL_STRING("test1", ctx.argv[0].c_str());
        TEST_ASSERT_EQUAL(1, ctx.argv.size());
        TEST_ASSERT_EQUAL(0, command_calls);
        ++command_calls;
    });

    add("test2", [](CommandContext&& ctx) {
        TEST_ASSERT_EQUAL_STRING("test2", ctx.argv[0].c_str());
        TEST_ASSERT_EQUAL(1, ctx.argv.size());
        TEST_ASSERT_EQUAL(1, command_calls);
        ++command_calls;
    });

    add("test3", [](CommandContext&& ctx) {
        TEST_ASSERT_EQUAL_STRING("test3", ctx.argv[0].c_str());
        TEST_ASSERT_EQUAL(1, ctx.argv.size());
        TEST_ASSERT_EQUAL(2, command_calls);
        ++command_calls;
    });

    const char input[] = "test1\r\ntest2\r\ntest3\r\n";
    TEST_ASSERT(api_find_and_call(input, DefaultOutput));
    TEST_ASSERT_EQUAL(3, command_calls);
}

void test_command() {
    static int counter = 0;

    add("test.command", [](CommandContext&& ctx) {
        TEST_ASSERT_EQUAL_MESSAGE(1, ctx.argv.size(),
            "Command without args should have argc == 1");
        ++counter;
    });

    const char crlf[] = "test.command\r\n";
    TEST_ASSERT(find_and_call(crlf, DefaultOutput));
    TEST_ASSERT_EQUAL_MESSAGE(1, counter,
        "`test.command` cannot be called more than one time");

    TEST_ASSERT(find_and_call(crlf, DefaultOutput));
    TEST_ASSERT_EQUAL_MESSAGE(2, counter,
        "`test.command` cannot be called more than two times");

    const char lf[] = "test.command\n";
    TEST_ASSERT(find_and_call(lf, DefaultOutput));
    TEST_ASSERT_EQUAL_MESSAGE(3, counter,
        "`test.command` cannot be called more than three times");
}

// Ensure that we can properly handle arguments
void test_command_args() {
    static bool waiting = false;

    add("test.command.arg1", [](CommandContext&& ctx) {
        TEST_ASSERT_EQUAL(2, ctx.argv.size());
        waiting = false;
    });

    add("test.command.arg1_empty", [](CommandContext&& ctx) {
        TEST_ASSERT_EQUAL(2, ctx.argv.size());
        TEST_ASSERT(!ctx.argv[1].length());
        waiting = false;
    });

    waiting = true;

    PrintString out(64);
    const char empty[] = "test.command.arg1_empty \"\"\r\n";
    const auto result = find_and_call(empty, out);
    printf("%s\n", out.c_str());
    TEST_ASSERT(result);
    TEST_ASSERT(!waiting);

    waiting = true;

    const char one_arg[] = "test.command.arg1 test\r\n";
    TEST_ASSERT(find_and_call(one_arg, DefaultOutput));
    TEST_ASSERT(!waiting);
}

// both \r\n and \n are valid line separators
void test_new_line() {
    {
        const auto result = parse_line("test.new.line\r\n");
        TEST_ASSERT_EQUAL(1, result.argv.size());
        TEST_ASSERT_EQUAL_STRING("test.new.line", result.argv[0].c_str());
    }

    {
        const auto result = parse_line("test.new.line\n");
        TEST_ASSERT_EQUAL(1, result.argv.size());
        TEST_ASSERT_EQUAL_STRING("test.new.line", result.argv[0].c_str());
    }

    {
        const auto result = parse_line("test.new.line\r");
        TEST_ASSERT_EQUAL_STRING("UnexpectedLineEnd",
            parser::error(result.error).c_str());
        TEST_ASSERT_EQUAL(0, result.argv.size());
    }
}

// various parser errors related to quoting
void test_quotes() {
    {
        const auto result = parse_line("test.quotes \"quote that does not\"feel right");
        TEST_ASSERT_EQUAL_STRING("NoSpaceAfterQuote",
            parser::error(result.error).c_str());
        TEST_ASSERT_EQUAL(0, result.argv.size());
    }

    {
        const auto result = parse_line("test.quotes \"quote that does not line break\"");
        TEST_ASSERT_EQUAL_STRING("NoSpaceAfterQuote",
            parser::error(result.error).c_str());
        TEST_ASSERT_EQUAL(0, result.argv.size());
    }

    {
        const auto result = parse_line("test.quotes \"quote without a pair\r\n");
        TEST_ASSERT_EQUAL_STRING("UnterminatedQuote",
            parser::error(result.error).c_str());
        TEST_ASSERT_EQUAL(0, result.argv.size());
    }

    {
        const auto result = parse_line("test.quotes 'quote without a pair\r\n");
        TEST_ASSERT_EQUAL_STRING("UnterminatedQuote",
            parser::error(result.error).c_str());
        TEST_ASSERT_EQUAL(0, result.argv.size());
    }

    {
        const auto result = parse_line("test.quotes ''\r\n");
        TEST_ASSERT_EQUAL(2, result.argv.size());
    }

    {
        const auto result = parse_line("test.quotes \"\"\r\n");
        TEST_ASSERT_EQUAL(2, result.argv.size());
    }
}

// we specify that commands lowercase == UPPERCASE
// last registered one should be called, we don't check for duplicates at this time
void test_case_insensitive() {
    add("test.lowercase1", [](CommandContext&&) {
        TEST_FAIL_MESSAGE("`test.lowercase1` was registered first, but there's another function by the same name. This should not be called");
    });

    add("TEST.LOWERCASE1", [](CommandContext&&) {
        __asm__ volatile ("nop");
    });

    const char input[] = "TeSt.lOwErCaSe1\r\n";
    TEST_ASSERT(find_and_call(input, DefaultOutput));
}

// We can use command ctx.output to send something back into the stream
void test_output() {
    add("test.output", [](CommandContext&& ctx) {
        if (ctx.argv.size() == 2) {
            ctx.output.print(ctx.argv[1]);
        }
    });

    const char input[] = "test.output test1234567890\r\n";

    PrintString output(64);
    TEST_ASSERT(find_and_call(input, output));

    const char match[] = "test1234567890";
    TEST_ASSERT_EQUAL_STRING(match, output.c_str());
}

// un-buffered view returning multiple times until strings are exhausted
void test_line_view() {
    const char input[] = "one\r\ntwo\r\nthree\r\n";
    LineView view(input);

    const auto one = view.line();
    TEST_ASSERT_EQUAL_STRING("one\r\n",
        one.toString().c_str());

    const auto two = view.line();
    TEST_ASSERT_EQUAL_STRING("two\r\n",
        two.toString().c_str());

    const auto three = view.line();
    TEST_ASSERT_EQUAL_STRING("three\r\n",
        three.toString().c_str());

    TEST_ASSERT_EQUAL(0, view.line().length());
}

// Ensure that we keep buffering when input is incomplete
void test_line_buffer() {
    const char input[] =
        "aaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaa";

    LineBuffer<256> buffer;
    buffer.append(input);

    TEST_ASSERT_EQUAL(buffer.size(), __builtin_strlen(input));
    TEST_ASSERT_EQUAL(0, buffer.line().line.length());

    buffer.append("\r\n");
    TEST_ASSERT_EQUAL(__builtin_strlen(input) + 2,
        buffer.line().line.length());
}

// Ensure that when buffer overflows, we set 'overflow' flags
// on both the buffer and the returned 'line' result
void test_line_buffer_overflow() {
    using Buffer = LineBuffer<16>;
    static_assert(Buffer::capacity() == 16, "");

    Buffer buffer;
    TEST_ASSERT(buffer.size() == 0);
    TEST_ASSERT(!buffer.overflow());

    // verify our expansion works, buffer needs to overflow
    std::array<char, (Buffer::capacity() * 2) + 2> data;
    std::fill(std::begin(data), std::end(data), 'd');
    data.back() = '\n';

    buffer.append(data.data(), data.size());
    TEST_ASSERT(buffer.overflow());

    const auto result = buffer.line();
    TEST_ASSERT(result.overflow);

    TEST_ASSERT(buffer.size() == 0);
    TEST_ASSERT(!buffer.overflow());

    // TODO: can't compare string_view directly, not null terminated
    const auto line = result.line.toString();
    TEST_ASSERT_EQUAL_STRING("d\n", line.c_str());
    TEST_ASSERT(line.length() > 0);
}

// When input has multiple 'new-line' characters, generated result only has one line at a time
void test_line_buffer_multiple() {
    LineBuffer<64> buffer;

    constexpr auto First = StringView("first\n");
    buffer.append(First);

    constexpr auto Second = StringView("second\n");
    buffer.append(Second);

    TEST_ASSERT_EQUAL(First.length() + Second.length(),
            buffer.size());
    TEST_ASSERT(!buffer.overflow());

    // both entries remain in the buffer and are available
    // if we don't touch the buffer via another append().
    // (in theory, could also add refcount... right now seems like an overkill)

    const auto first = buffer.line();
    TEST_ASSERT(buffer.size() > 0);
    TEST_ASSERT_EQUAL(First.length(),
            first.line.length());
    TEST_ASSERT(First == first.line);

    // second entry resets everything
    const auto second = buffer.line();
    TEST_ASSERT_EQUAL(0, buffer.size());
    TEST_ASSERT_EQUAL(Second.length(),
            second.line.length());
    TEST_ASSERT(Second == second.line);
}

} // namespace
} // namespace test
} // namespace terminal
} // namespace espurna

// When adding test functions, don't forget to add RUN_TEST(...) in the main()

int main(int, char**) {
    UNITY_BEGIN();

    using namespace espurna::terminal::test;
    RUN_TEST(test_command);
    RUN_TEST(test_command_args);
    RUN_TEST(test_parse_overlap);
    RUN_TEST(test_commands_array);
    RUN_TEST(test_multiple_commands);
    RUN_TEST(test_hex_codes);
    RUN_TEST(test_quotes);
    RUN_TEST(test_case_insensitive);
    RUN_TEST(test_output);
    RUN_TEST(test_new_line);
    RUN_TEST(test_line_view);
    RUN_TEST(test_line_buffer);
    RUN_TEST(test_line_buffer_overflow);
    RUN_TEST(test_line_buffer_multiple);

    return UNITY_END();
}
