#include <unity.h>
#include <Arduino.h>

#include <settings_embedis.h>

#include <array>

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

void test_basic() {
    TestStorageHandler instance;

    // ensure insert works
    std::vector<std::pair<String, String>> kvs;
    for (size_t index = 0; index < 5; ++index) {
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

    // ensure we can remove keys (TODO: randomize this?)
    do {
        auto key = kvs.front().first;
        kvs.erase(kvs.begin());
        TEST_ASSERT(instance.storage.del(key));

        auto result = instance.storage.get(key);
        TEST_ASSERT_FALSE(static_cast<bool>(result));
    } while (kvs.size());

}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_basic);
    UNITY_END();
}
