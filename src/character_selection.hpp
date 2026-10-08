#pragma once

#include <cstdint>

namespace kingdom {

// Persisted IDs: append new choices; never renumber existing entries.
enum class CharacterSelection : int64_t { None = 0, Xion = 1 };
inline constexpr const char* kCharacterOptions[] = {"None", "Xion"};

constexpr bool validCharacterSelection(int64_t value) noexcept {
    return value == static_cast<int64_t>(CharacterSelection::None) ||
           value == static_cast<int64_t>(CharacterSelection::Xion);
}
constexpr CharacterSelection characterSelectionFromId(int64_t value) noexcept {
    return value == static_cast<int64_t>(CharacterSelection::Xion)
        ? CharacterSelection::Xion : CharacterSelection::None;
}

} // namespace kingdom
