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
        blob.fill(0);
    }

    uint8_t read(size_t index) override {
        return blob[index];
    }

    void write(size_t index, uint8_t value) override {
        blob[index] = value;
    }

    size_t size() override {
        return Size;
    }

    std::array<uint8_t, Size> blob;
};

} // namespace embedis
} // namespace settings

struct TestStorageHandler {
    TestStorageHandler() :
        storage(source)
    {}

    settings::embedis::RawStorage storage;
    settings::embedis::StaticArraySource<1024> source;
};

void test_remove_randomized() {

    // ensure we can remove keys in any order
    // 5 -> 120 combinations
    constexpr size_t KeysNumber = 5;

    std::vector<std::pair<String, String>> kvs;
    for (size_t index = 0; index < KeysNumber; ++index) {
        kvs.push_back(std::make_pair(
            String("key") + String(index),
            String("val") + String(index)
        ));
    }

    // generate indexes array to allow us to reference keys at random
    TestStorageHandler instance;
    std::array<size_t, KeysNumber> indexes;
    std::iota(indexes.begin(), indexes.end(), 0);

    // - insert keys sequentially
    // - remove keys based on the order provided by next_permutation()
    do {
        instance.storage._cursor.position = 1024;
        instance.storage._cursor.end = 1024;
        for (auto& kv : kvs) {
            TEST_ASSERT(instance.storage.set(kv.first, kv.second));
        }

        for (auto index : indexes) {
            auto key = kvs[index].first;
            TEST_ASSERT(instance.storage.del(key));
            auto result = instance.storage.get(key);
            TEST_ASSERT_FALSE(static_cast<bool>(result));
        }
    } while (std::next_permutation(indexes.begin(), indexes.end()));

}

void test_basic() {
    TestStorageHandler instance;

    constexpr size_t KeysNumber = 5;

    // ensure insert works
    std::vector<std::pair<String, String>> kvs;
    for (size_t index = 0; index < KeysNumber; ++index) {
        kvs.push_back(std::make_pair(
            String("key") + String(index),
            String("val") + String(index)
        ));
    }

    for (auto& kv : kvs) {
        instance.storage.set(kv.first, kv.second);
    }

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
    UNITY_END();
}
