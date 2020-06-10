#pragma GCC diagnostic warning "-Wall"

#include <unity.h>
#include <Arduino.h>

#include <settings_embedis.h>

#include <array>
#include <algorithm>
#include <numeric>

namespace settings {
namespace embedis {

template <size_t Size>
struct StaticArraySource final : public RawStorage::SourceBase {
    StaticArraySource() {
        blob.fill(0xff);
    }

    uint8_t read(size_t index) override {
        TEST_ASSERT_LESS_THAN(Size, index);
        return blob[index];
    }

    void write(size_t index, uint8_t value) override {
        TEST_ASSERT_LESS_THAN(Size, index);
        blob[index] = value;
    }

    size_t size() override {
        return Size;
    }

    std::array<uint8_t, Size> blob;
};

} // namespace embedis
} // namespace settings

template <size_t Size>
struct StorageHandler {
    StorageHandler() :
        storage(source)
    {}

    settings::embedis::StaticArraySource<Size> source;
    settings::embedis::RawStorage storage;
};

struct TestSequentialKvGenerator {

    using kv = std::pair<String, String>;

    kv next() {
        auto index = _index++;
        auto res = std::make_pair(
            String("key") + String(index),
            String("val") + String(index)
        );
        TEST_ASSERT(_last.first != res.first);
        TEST_ASSERT(_last.second != res.second);
        return (_last = res);
    }

    std::vector<kv> make(size_t size) {;
        std::vector<kv> res;
        for (size_t index = 0; index < size; ++index) {
            res.push_back(next());
        }
        return res;
    }

    kv _last;
    size_t _index { 0 };

};

// ----------------------------------------------------------------------------

using TestStorageHandler = StorageHandler<1024>;

template <typename T>
void check_kv(T& instance, const String& key, const String& value) {
    auto result = instance.storage.get(key);
    TEST_ASSERT_MESSAGE(static_cast<bool>(result), key.c_str());
    TEST_ASSERT(result.value.length());
    TEST_ASSERT_EQUAL_STRING(value.c_str(), result.value.c_str());
};

void test_perseverance() {

    // ensure we can handle setting the same key
    using storage_type = StorageHandler<128>;
    using blob_type = decltype(std::declval<storage_type>().source.blob);

    // xxx: implementation detail?
    // can we avoid blob modification when value is the same as the existing one
    {
        storage_type instance;

        TEST_ASSERT(instance.storage.set("key", "value"));
        TEST_ASSERT(instance.storage.set("another", "keyvalue"));
        blob_type snapshot(instance.source.blob);

        TEST_ASSERT(instance.storage.set("key", "value"));
        TEST_ASSERT(snapshot == instance.source.blob);
    }

    // xxx: pointless implementation detail?
    // can we re-use existing 'value' storage and avoid data-shift
    {
        storage_type instance;

        // insert in a specific order, change middle
        TEST_ASSERT(instance.storage.set("aaa", "bbb"));
        TEST_ASSERT(instance.storage.set("cccc", "dd"));
        TEST_ASSERT(instance.storage.set("ee", "fffff"));
        TEST_ASSERT(instance.storage.set("cccc", "ff"));
        blob_type before(instance.source.blob);

        // purge, insert again with updated values
        TEST_ASSERT(instance.storage.del("aaa"));
        TEST_ASSERT(instance.storage.del("cccc"));
        TEST_ASSERT(instance.storage.del("ee"));

        TEST_ASSERT(instance.storage.set("aaa", "bbb"));
        TEST_ASSERT(instance.storage.set("cccc", "ff"));
        TEST_ASSERT(instance.storage.set("ee", "fffff"));
        blob_type after(instance.source.blob);

        TEST_ASSERT(before == after);
    }
}

void test_overflow() {

    // ensure we don't blow up when trying to manipulate filled up kv storage
    StorageHandler<16> instance;

    TEST_ASSERT(instance.storage.set("a", "b"));
    TEST_ASSERT(instance.storage.set("c", "d"));

    TEST_ASSERT_EQUAL(2, instance.storage.keys());
    TEST_ASSERT_FALSE(instance.storage.set("e", "f"));

    TEST_ASSERT(instance.storage.del("a"));

    TEST_ASSERT_EQUAL(1, instance.storage.keys());
    TEST_ASSERT(instance.storage.set("e", "f"));

    TEST_ASSERT_EQUAL(2, instance.storage.keys());

    check_kv(instance, "e", "f");
    check_kv(instance, "c", "d");

}

void test_small_gaps() {

    // ensure we can intemix empty and non-empty values
    TestStorageHandler instance;

    TEST_ASSERT(instance.storage.set("key", "value"));
    TEST_ASSERT(instance.storage.set("empty", ""));
    TEST_ASSERT(instance.storage.set("empty_again", ""));
    TEST_ASSERT(instance.storage.set("finally", "avalue"));

    auto check_empty = [&instance](const String& key) {
        auto result = instance.storage.get(key);
        TEST_ASSERT(static_cast<bool>(result));
        TEST_ASSERT_FALSE(result.value.length());
    };

    check_empty("empty_again");
    check_empty("empty");
    check_empty("empty_again");
    check_empty("empty");

    auto check_value = [&instance](const String& key, const String& value) {
        auto result = instance.storage.get(key);
        TEST_ASSERT(static_cast<bool>(result));
        TEST_ASSERT(result.value.length());
        TEST_ASSERT_EQUAL_STRING(value.c_str(), result.value.c_str());
    };

    check_value("finally", "avalue");
    check_value("key", "value");

}

void test_remove_randomized() {

    // ensure we can remove keys in any order
    // 8 seems like a good number to stop on, 9 will spend ~10seconds
    // TODO: seems like a good start benchmarking read / write performance?
    constexpr size_t KeysNumber = 8;

    TestSequentialKvGenerator generator;
    auto kvs = generator.make(KeysNumber);

    // generate indexes array to allow us to reference keys at random
    TestStorageHandler instance;
    std::array<size_t, KeysNumber> indexes;
    std::iota(indexes.begin(), indexes.end(), 0);

    // - insert keys sequentially
    // - remove keys based on the order provided by next_permutation()
    size_t index = 0;
    do {
        TEST_ASSERT(0 == instance.storage.keys());
        for (auto& kv : kvs) {
            TEST_ASSERT(instance.storage.set(kv.first, kv.second));
        }

        for (auto index : indexes) {
            auto key = kvs[index].first;
            TEST_ASSERT(instance.storage.del(key));
            auto result = instance.storage.get(key);
            TEST_ASSERT_FALSE(static_cast<bool>(result));
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
        instance.storage.set(kv.first, kv.second);
    }

    // and we can retrieve keys back
    for (auto& kv : kvs) {
        auto result = instance.storage.get(kv.first);
        TEST_ASSERT(static_cast<bool>(result));
        TEST_ASSERT_EQUAL_STRING(kv.second.c_str(), result.value.c_str());
    }

}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_basic);
    RUN_TEST(test_remove_randomized);
    RUN_TEST(test_small_gaps);
    RUN_TEST(test_overflow);
    RUN_TEST(test_perseverance);
    UNITY_END();
}
