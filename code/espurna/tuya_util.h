namespace TuyaDimmer {

    template <typename T>
    class States {

        public:

            struct Container {
                uint8_t dp;
                T value;
            };


            States(size_t capacity) :
                _capacity(capacity)
            {
                _states.reserve(capacity);
            }

            bool update(const uint8_t dp, const T value) {
                auto found = std::find_if(_states.begin(), _states.end(), [dp](const Container& internal) {
                    return dp == internal.dp;
                });

                if (found != _states.end()) {
                    if (found->value != value) {
                        found->value = value;
                        _changed = true;
                        return true;
                    }
                }

                return false;
            }

            void pushOrUpdate(const uint8_t dp, const T value) {
                if (_states.size() >= _capacity) return;
                if (!update(dp, value)) {
                    _changed = true;
                    _states.emplace_back(States::Container{dp, value});
                }
            }

            bool changed() {
                bool res = _changed;
                if (_changed) _changed = false;
                return res;
            }

            Container& operator[] (const size_t n) {
                return _states[n];
            }

            size_t size() {
                return _states.size();
            }

            size_t capacity() {
                return _capacity;
            }

        private:
            bool _changed = false;
            size_t _capacity = 0;
            std::vector<Container> _states;
    };

    class DiscoveryTimeout {
        public:
            DiscoveryTimeout(uint32_t start, uint32_t timeout) :
                _start(start),
                _timeout(timeout)
            {}

            DiscoveryTimeout(uint32_t timeout) :
                DiscoveryTimeout(millis(), timeout)
            {}

            operator bool() {
                return (millis() - _start > _timeout);
            }

            void feed() {
                _start = millis();
            }

        private:
            uint32_t _start;
            const uint32_t _timeout;
    };

}
