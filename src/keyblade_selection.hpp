#pragma once

#include <cstdint>

namespace kingdom {

// Persisted IDs: append new choices; never renumber existing entries.
enum class KeybladeSelection : int64_t { None = 0, KingdomKey = 1 };
inline constexpr const char* kKeybladeOptions[] = {"None", "Kingdom Key"};

constexpr bool validKeybladeSelection(int64_t value) noexcept {
    return value == static_cast<int64_t>(KeybladeSelection::None) ||
           value == static_cast<int64_t>(KeybladeSelection::KingdomKey);
}
constexpr KeybladeSelection keybladeSelectionFromId(int64_t value) noexcept {
    return value == static_cast<int64_t>(KeybladeSelection::KingdomKey)
        ? KeybladeSelection::KingdomKey : KeybladeSelection::None;
}

} // namespace kingdom
