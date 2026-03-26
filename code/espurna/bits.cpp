#include "bits_range.re.ipp"

namespace espurna {
namespace bits {

String Range::toString() const {
    char tmp[SizeMax]{};

    for (size_t bit = 0; bit < _mask.size(); ++bit) {
        tmp[bit] = _mask[bit] ? '1' : '0';
    }

    String out;
    out.concat(&tmp[0], sizeof(tmp));

    return out;
}

void Range::_fill_inverse(uint8_t begin, uint8_t end, uint8_t repeat) {
    if (repeat == 1) {
        _mask |= fill_u64_inverse(begin, end + 1);
        return;
    }

    for (int n = begin; n <= _end; n += repeat) {
        _mask[n] = true;
    }

    for (int n = _begin; n <= end; n += repeat) {
        _mask[n] = true;
    }
}

void Range::_fill(uint8_t begin, uint8_t end, uint8_t repeat) {
    if (repeat == 1) {
        _mask |= fill_u64(begin, end + 1);
        return;
    }

    for (int n = begin; n <= end; n += repeat) {
        _mask[n] = true;
    }
}

constexpr size_t Range::SizeMax;

} // namespace bits
} // namespace espurna
