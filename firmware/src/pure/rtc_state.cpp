#include "pure/rtc_state.h"

#include <cstring>

namespace epb {

uint32_t crc32(const uint8_t* data, size_t length) {
    // 0xEDB88320 is the standard CRC-32 polynomial, written bit-reversed
    // because this implementation works from the lowest bit upwards.
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            // If the lowest bit is set, shift and "subtract" (XOR) the
            // polynomial; otherwise just shift. That is one step of binary
            // long division.
            const uint32_t lowBitSet = crc & 1u;
            crc >>= 1;
            if (lowBitSet != 0) {
                crc ^= 0xEDB88320u;
            }
        }
    }
    return ~crc;
}

namespace {

uint32_t checksumOf(const RtcState& state) {
    // Everything except the crc member itself.
    return crc32(reinterpret_cast<const uint8_t*>(&state), offsetof(RtcState, crc));
}

}  // namespace

RtcState makeDefaultRtcState() {
    RtcState state;
    std::memset(&state, 0, sizeof(state));  // Every field 0, every string empty.
    state.magic = kRtcMagic;
    state.version = kRtcVersion;
    sealRtcState(&state);
    return state;
}

void sealRtcState(RtcState* state) {
    state->crc = checksumOf(*state);
}

bool isRtcStateValid(const RtcState& state) {
    return state.magic == kRtcMagic && state.version == kRtcVersion && state.crc == checksumOf(state);
}

}  // namespace epb
