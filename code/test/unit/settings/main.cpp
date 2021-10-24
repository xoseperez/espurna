#include <unity.h>
#include <Arduino.h>

#pragma GCC diagnostic warning "-Wall"
#pragma GCC diagnostic warning "-Wextra"
#pragma GCC diagnostic warning "-Wstrict-aliasing"
#pragma GCC diagnostic warning "-Wpointer-arith"
#pragma GCC diagnostic warning "-Wstrict-overflow=5"

#include <settings_embedis.h>

#include <array>
#include <algorithm>
#include <numeric>

namespace settings {
namespace embedis {

// TODO: either
// - throw exception and print backtrace to signify which method caused the out-of-bounds read or write
// - use a custom macro to supply (instance?) object with the correct line number
template <typename T>
struct StaticArrayStorage {

    explicit StaticArrayStorage(T& blob) :
        _blob(blob),
        _size(blob.size())
    {}

    uint8_t read(size_t index) const {
        TEST_ASSERT_LESS_THAN(_size, index);
        return _blob[index];
    }

    void write(size_t index, uint8_t value) {
        TEST_ASSERT_LESS_THAN(_size, index);
        _blob[index] = value;
    }

    void commit() {
    }

    T& _blob;
    const size_t _size;

};

} // namespace embedis
} // namespace settings

template <size_t Size>
struct StorageHandler {
    using array_type = std::array<uint8_t, Size>;
    using storage_type = settings::embedis::StaticArrayStorage<array_type>;
    using kvs_type = settings::embedis::KeyValueStore<storage_type>;

    StorageHandler() :
        kvs(std::move(storage_type{blob}), 0, Size)
    {
        blob.fill(0xff);
    }

    array_type blob;
    kvs_type kvs;
};

// generate stuff depending on the mode
// - Indexed: key1:val1, key2:val2, ...
// - IncreasingLength: k:v, kk:vv, ...
struct TestSequentialKvGenerator {

    using kv = std::pair<String, String>;
    enum class Mode {
        Indexed,
        IncreasingLength
    };

    TestSequentialKvGenerator() = default;
    explicit TestSequentialKvGenerator(Mode mode) :
        _mode(mode)
    {}

    const kv& next() {
        auto index = _index++;

        _current.first = "";
        _current.second = "";

        switch (_mode) {
        case Mode::Indexed:
            _current.first = String("key") + String(index);
            _current.second = String("val") + String(index);
            break;
        case Mode::IncreasingLength: {
            size_t sizes = _index;
            _current.first.reserve(sizes);
            _current.second.reserve(sizes);

            do {
                _current.first += "k";
                _current.second += "v";
            } while (--sizes);
            break;
        }
        }
        TEST_ASSERT(_last.first != _current.first);
        TEST_ASSERT(_last.second != _current.second);

        return (_last = _current);
    }

    std::vector<kv> make(size_t size) {;
        std::vector<kv> res;
        for (size_t index = 0; index < size; ++index) {
            res.push_back(next());
        }
        return res;
    }

    kv _current;
    kv _last;

    Mode _mode { Mode::Indexed };
    size_t _index { 0 };

};

// ----------------------------------------------------------------------------

using TestStorageHandler = StorageHandler<1024>;

template <typename T>
void check_kv(T& instance, const String& key, const String& value) {
    auto result = instance.kvs.get(key);
    TEST_ASSERT_MESSAGE(static_cast<bool>(result), key.c_str());
    TEST_ASSERT(result.length());
    TEST_ASSERT_EQUAL_STRING(value.c_str(), result.c_str());
};

void test_sizes() {

    // empty storage is still manageble, it just does not work :)
    {
        StorageHandler<0> empty;
        TEST_ASSERT_EQUAL(0, empty.kvs.count());
        TEST_ASSERT_FALSE(empty.kvs.set("cannot", "happen"));
        TEST_ASSERT_FALSE(static_cast<bool>(empty.kvs.get("cannot")));
    }

    // some hard-coded estimates to notify us about internal changes
    {
        StorageHandler<16> instance;
        TEST_ASSERT_EQUAL(0, instance.kvs.count());
        TEST_ASSERT_EQUAL(16, instance.kvs.available());
        TEST_ASSERT_EQUAL(0, settings::embedis::estimate("", "123456"));
        TEST_ASSERT_EQUAL(16, settings::embedis::estimate("123456", "123456"));
        TEST_ASSERT_EQUAL(10, settings::embedis::estimate("123", "123"));
        TEST_ASSERT_EQUAL(7, settings::embedis::estimate("345", ""));
        TEST_ASSERT_EQUAL(0, settings::embedis::estimate("", ""));
        TEST_ASSERT_EQUAL(5, settings::embedis::estimate("1", ""));
    }

}

void test_longkey() {

    TestStorageHandler instance;
    const auto estimate = instance.kvs.size() - 6;

    String key;
    key.reserve(estimate);
    for (size_t n = 0; n < estimate; ++n) {
        key += 'a';
    }

    TEST_ASSERT(instance.kvs.set(key, ""));
    auto result = instance.kvs.get(key);
    TEST_ASSERT(static_cast<bool>(result));

}

void test_perseverance() {

    // ensure we can handle setting the same key
    using storage_type = StorageHandler<128>;
    using blob_type = decltype(std::declval<storage_type>().blob);

    // xxx: implementation detail?
    // can we avoid blob modification when value is the same as the existing one
    {
        storage_type instance;
        blob_type original(instance.blob);

        TEST_ASSERT(instance.kvs.set("key", "value"));
        TEST_ASSERT(instance.kvs.set("another", "keyvalue"));
        TEST_ASSERT(original != instance.blob);
        blob_type snapshot(instance.blob);

        TEST_ASSERT(instance.kvs.set("key", "value"));
        TEST_ASSERT(snapshot == instance.blob);
    }

    // xxx: pointless implementation detail?
    // can we re-use existing 'value' storage and avoid data-shift
    {
        storage_type instance;
        blob_type original(instance.blob);

        // insert in a specific order, change middle
        TEST_ASSERT(instance.kvs.set("aaa", "bbb"));
        TEST_ASSERT(instance.kvs.set("cccc", "dd"));
        TEST_ASSERT(instance.kvs.set("ee", "fffff"));
        TEST_ASSERT(instance.kvs.set("cccc", "ff"));
        TEST_ASSERT(original != instance.blob);
        blob_type before(instance.blob);

        // purge, insert again with updated values
        TEST_ASSERT(instance.kvs.del("aaa"));
        TEST_ASSERT(instance.kvs.del("cccc"));
        TEST_ASSERT(instance.kvs.del("ee"));

        TEST_ASSERT(instance.kvs.set("aaa", "bbb"));
        TEST_ASSERT(instance.kvs.set("cccc", "ff"));
        TEST_ASSERT(instance.kvs.set("ee", "fffff"));
        blob_type after(instance.blob);

        TEST_ASSERT(original != before);
        TEST_ASSERT(original != after);
        TEST_ASSERT(before == after);
    }
}

template <size_t Size>
struct test_overflow_runner {
    void operator ()() const {
        StorageHandler<Size> instance;

        TEST_ASSERT(instance.kvs.set("a", "b"));
        TEST_ASSERT(instance.kvs.set("c", "d"));

        TEST_ASSERT_EQUAL(2, instance.kvs.count());
        TEST_ASSERT_FALSE(instance.kvs.set("e", "f"));

        TEST_ASSERT(instance.kvs.del("a"));

        TEST_ASSERT_EQUAL(1, instance.kvs.count());
        TEST_ASSERT(instance.kvs.set("e", "f"));

        TEST_ASSERT_EQUAL(2, instance.kvs.count());

        check_kv(instance, "e", "f");
        check_kv(instance, "c", "d");
    }
};

void test_overflow() {
    // slightly more that available, but we cannot fit the key
    test_overflow_runner<16>();

    // no more space
    test_overflow_runner<12>();
}

void test_small_gaps() {

    // ensure we can intemix empty and non-empty values
    TestStorageHandler instance;

    TEST_ASSERT(instance.kvs.set("key", "value"));
    TEST_ASSERT(instance.kvs.set("empty", ""));
    TEST_ASSERT(instance.kvs.set("empty_again", ""));
    TEST_ASSERT(instance.kvs.set("finally", "avalue"));

    auto check_empty = [&instance](const String& key) {
        auto result = instance.kvs.get(key);
        TEST_ASSERT(static_cast<bool>(result));
        TEST_ASSERT_FALSE(result.length());
    };

    check_empty("empty_again");
    check_empty("empty");
    check_empty("empty_again");
    check_empty("empty");

    auto check_value = [&instance](const String& key, const String& value) {
        auto result = instance.kvs.get(key);
        TEST_ASSERT(static_cast<bool>(result));
        TEST_ASSERT(result.length());
        TEST_ASSERT_EQUAL_STRING(value.c_str(), result.c_str());
    };

    check_value("finally", "avalue");
    check_value("key", "value");

}

void test_remove_randomized() {

    // ensure we can remove keys in any order
    // 8 seems like a good number to stop on, 9 will spend ~10seconds
    // TODO: seems like a good start benchmarking read / write performance?
    constexpr size_t KeysNumber = 8;

    TestSequentialKvGenerator generator(TestSequentialKvGenerator::Mode::IncreasingLength);
    auto kvs = generator.make(KeysNumber);

    // generate indexes array to allow us to reference keys at random
    TestStorageHandler instance;
    std::array<size_t, KeysNumber> indexes;
    std::iota(indexes.begin(), indexes.end(), 0);

    // - insert keys sequentially
    // - remove keys based on the order provided by next_permutation()
    size_t index = 0;
    do {
        TEST_ASSERT_EQUAL(0, instance.kvs.count());

        for (auto& kv : kvs) {
            TEST_ASSERT(instance.kvs.set(kv.first, kv.second));
        }

        for (auto index : indexes) {
            auto key = kvs[index].first;
            TEST_ASSERT(static_cast<bool>(instance.kvs.get(key)));
            TEST_ASSERT(instance.kvs.del(key));
            TEST_ASSERT_FALSE(static_cast<bool>(instance.kvs.get(key)));
        }

        index++;
    } while (std::next_permutation(indexes.begin(), indexes.end()));

    String message("- keys: ");
    message += KeysNumber;
    message += ", permutations: ";
    message += index;
    TEST_MESSAGE(message.c_str());

}

void test_basic() {
    TestStorageHandler instance;

    constexpr size_t KeysNumber = 5;

    // ensure insert works
    TestSequentialKvGenerator generator;
    auto kvs = generator.make(KeysNumber);

    for (auto& kv : kvs) {
        instance.kvs.set(kv.first, kv.second);
    }

    // and we can retrieve keys back
    for (auto& kv : kvs) {
        auto result = instance.kvs.get(kv.first);
        TEST_ASSERT(static_cast<bool>(result));
        TEST_ASSERT_EQUAL_STRING(kv.second.c_str(), result.c_str());
    }

}

void test_storage() {

    constexpr size_t Size = 32;
    StorageHandler<Size> instance;

    // empty keys are invalid
    TEST_ASSERT_FALSE(instance.kvs.set("", "value1"));
    TEST_ASSERT_FALSE(instance.kvs.del(""));

    // ...and both keys are not yet set
    TEST_ASSERT_FALSE(instance.kvs.del("key1"));
    TEST_ASSERT_FALSE(instance.kvs.del("key2"));

    // some different ways to set keys
    TEST_ASSERT(instance.kvs.set("key1", "value0"));
    TEST_ASSERT_EQUAL(1, instance.kvs.count());
    TEST_ASSERT(instance.kvs.set("key1", "value1"));
    TEST_ASSERT_EQUAL(1, instance.kvs.count());

    TEST_ASSERT(instance.kvs.set("key2", "value_old"));
    TEST_ASSERT_EQUAL(2, instance.kvs.count());
    TEST_ASSERT(instance.kvs.set("key2", "value2"));
    TEST_ASSERT_EQUAL(2, instance.kvs.count());

    auto kvsize = settings::embedis::estimate("key1", "value1");
    TEST_ASSERT_EQUAL((Size - (2 * kvsize)), instance.kvs.available());

    // checking keys one by one by using a separate kvs object,
    // working on the same underlying data-store
    using storage_type = decltype(instance)::storage_type;
    using kvs_type = decltype(instance)::kvs_type;

    // - ensure we can operate with storage offsets
    // - test for internal length optimization that will overwrite the key in-place
    // - make sure we did not break the storage above
    // storage_type accepts reference to the blob, so we can seamlessly use the same
    // underlying data storage and share it between kvs instances
    {
        kvs_type slice(storage_type(instance.blob), (Size - kvsize), Size);
        TEST_ASSERT_EQUAL(1, slice.count());
        TEST_ASSERT_EQUAL(kvsize, slice.size());
        TEST_ASSERT_EQUAL(0, slice.available());
        auto result = slice.get("key1");
        TEST_ASSERT(static_cast<bool>(result));
        TEST_ASSERT_EQUAL_STRING("value1", result.c_str());
    }

    // ensure that right offset also works
    {
        kvs_type slice(storage_type(instance.blob), 0, (Size - kvsize));
        TEST_ASSERT_EQUAL(1, slice.count());
        TEST_ASSERT_EQUAL((Size - kvsize), slice.size());
        TEST_ASSERT_EQUAL((Size - kvsize - kvsize), slice.available());
        auto result = slice.get("key2");
        TEST_ASSERT(static_cast<bool>(result));
        TEST_ASSERT_EQUAL_STRING("value2", result.c_str());
    }

    // ensure offset does not introduce offset bugs
    // for instance, test in-place key overwrite by moving left boundary 2 bytes to the right
    {
        const auto available = instance.kvs.available();
        const auto offset = 2;

        TEST_ASSERT_GREATER_OR_EQUAL(offset, available);

        kvs_type slice(storage_type(instance.blob), offset, Size);
        TEST_ASSERT_EQUAL(2, slice.count());

        auto key1 = slice.get("key1");
        TEST_ASSERT(static_cast<bool>(key1));

        String updated(key1.ref());
        for (size_t index = 0; index < key1.length(); ++index) {
            updated[index] = 'A';
        }

        TEST_ASSERT(slice.set("key1", updated));
        TEST_ASSERT(slice.set("key2", updated));

        TEST_ASSERT_EQUAL(2, slice.count());

        auto check_key1 = slice.get("key1");
        TEST_ASSERT(static_cast<bool>(check_key1));
        TEST_ASSERT_EQUAL_STRING(updated.c_str(), check_key1.c_str());

        auto check_key2 = slice.get("key2");
        TEST_ASSERT(static_cast<bool>(check_key2));
        TEST_ASSERT_EQUAL_STRING(updated.c_str(), check_key2.c_str());

        TEST_ASSERT_EQUAL(available - offset, slice.available());
    }

}

void test_keys_iterator() {

    constexpr size_t Size = 32;
    StorageHandler<Size> instance;

    TEST_ASSERT_EQUAL(Size, instance.kvs.available());
    TEST_ASSERT_EQUAL(Size, instance.kvs.size());
    TEST_ASSERT(instance.kvs.set("key", "value"));
    TEST_ASSERT(instance.kvs.set("another", "thing"));

    // ensure we get the same order of keys when iterating via foreach
    std::vector<String> keys;
    instance.kvs.foreach([&keys](decltype(instance)::kvs_type::KeyValueResult&& kv) {
        keys.push_back(kv.key.read());
    });

    TEST_ASSERT_EQUAL(2, keys.size());
    TEST_ASSERT_EQUAL(2, instance.kvs.count());
    TEST_ASSERT_EQUAL_STRING("key", keys[0].c_str());
    TEST_ASSERT_EQUAL_STRING("another", keys[1].c_str());

}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_storage);
    RUN_TEST(test_keys_iterator);
    RUN_TEST(test_basic);
    RUN_TEST(test_remove_randomized);
    RUN_TEST(test_small_gaps);
    RUN_TEST(test_overflow);
    RUN_TEST(test_perseverance);
    RUN_TEST(test_longkey);
    RUN_TEST(test_sizes);
    return UNITY_END();
}
