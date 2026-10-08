#include "../src/keyblade_text.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
unsigned checks = 0;
void check(bool condition) {
    ++checks;
    if (!condition) {
        std::cerr << "FAIL at check " << checks << '\n';
        std::exit(1);
    }
}

std::vector<uint8_t> encoded(std::string text) {
    std::vector<uint8_t> result(text.begin(), text.end());
    result.push_back(0);
    return result;
}
std::vector<uint8_t> hex(const std::string& source) {
    std::vector<uint8_t> result;
    for (size_t i = 0; i < source.size(); i += 2)
        result.push_back(static_cast<uint8_t>(std::stoul(source.substr(i, 2), nullptr, 16)));
    return result;
}
MessageOverrideContext context(const std::vector<uint8_t>& bytes, uint16_t id = 0x18d) {
    return {0, id, MESSAGE_LANGUAGE_ENGLISH, bytes.data(), bytes.size()};
}
bool rewrite(const std::vector<uint8_t>& bytes, std::vector<uint8_t>& output, uint16_t id = 0x18d) {
    return kingdom::text::rewrite(context(bytes, id), output);
}

struct Registration { uint16_t group, id; uint8_t language; MessageOverrideFn callback; void* data; };
std::map<MessageOverrideHandle, Registration> registered;
MessageOverrideHandle nextHandle = 1;
unsigned attempts = 0, failOnAttempt = 0, removals = 0;
ModResult registerMessage(ModContext*, uint16_t group, uint16_t id, uint8_t language,
                          MessageOverrideFn callback, void* data, MessageOverrideHandle* handle) {
    ++attempts;
    *handle = 0;
    if (attempts == failOnAttempt) return MOD_UNAVAILABLE;
    *handle = nextHandle++;
    registered[*handle] = {group, id, language, callback, data};
    return MOD_OK;
}
ModResult removeMessage(ModContext*, MessageOverrideHandle handle) {
    check(registered.erase(handle) == 1);
    ++removals;
    return MOD_OK;
}
} // namespace

int main() {
    // Menu extraction bypasses MessageService: replace its bounded native span.
    // Guard bytes prove equal-size replacement does not touch adjacent UI data.
    std::array<char, 20> titleBuffer{};
    titleBuffer.fill('#');
    std::memcpy(titleBuffer.data() + 2, "Ordon Sword", 12);
    for (size_t capacity = 0; capacity < 12; ++capacity) {
        const auto original = titleBuffer;
        check(!kingdom::text::replacePlainTitle(0x18d, titleBuffer.data() + 2, capacity));
        check(titleBuffer == original);
    }
    check(!kingdom::text::replacePlainTitle(0x18d, nullptr, 12));
    check(!kingdom::text::replacePlainTitle(0x18e, titleBuffer.data() + 2, 12));
    check(kingdom::text::replacePlainTitle(0x18d, titleBuffer.data() + 2, 12));
    check(std::memcmp(titleBuffer.data() + 2, "Kingdom Key", 12) == 0);
    check(titleBuffer[0] == '#' && titleBuffer[1] == '#');
    for (size_t index = 14; index < titleBuffer.size(); ++index) check(titleBuffer[index] == '#');
    check(!kingdom::text::replacePlainTitle(0x18d, titleBuffer.data() + 2, 12));
    std::memcpy(titleBuffer.data() + 2, "Ordon SwordX", 12); // No terminator: refuse.
    check(!kingdom::text::replacePlainTitle(0x18d, titleBuffer.data() + 2, 12));
    std::memcpy(titleBuffer.data() + 2, "Master Sword", 13);
    check(!kingdom::text::replacePlainTitle(0x18d, titleBuffer.data() + 2, 13));
    std::memcpy(titleBuffer.data() + 2, "Schwert", 8); // Localized output: refuse.
    check(!kingdom::text::replacePlainTitle(0x18d, titleBuffer.data() + 2, 13));

    std::vector<uint8_t> output;
    const auto title = encoded("Ordon Sword");
    check(rewrite(title, output));
    check(output == encoded("Kingdom Key"));

    // Exact US pickup fixture: color, pacing and pause controls include NUL bytes.
    const auto pickup = hex("1a05000001596f7520676f7420746865201a06ff0000014f72646f6e2073776f7264"
        "1a06ff000000211a050000020a1a07000007000a5468697320697320612073776f7264206372616674656420"
        "6279205275736c2c0a74686520626573742073776f7264736d616e20696e204f72646f6e2c0a617320612074"
        "72696275746520746f2074686520726f79616c2066616d696c792e00");
    auto expected = pickup;
    const std::string oldName = "Ordon sword", newName = "Kingdom Key";
    const std::string pickupBytes(pickup.begin(), pickup.end());
    const auto phraseOffset = pickupBytes.find(oldName);
    check(phraseOffset != std::string::npos);
    std::copy(newName.begin(), newName.end(), expected.begin() + phraseOffset);
    check(rewrite(pickup, output, 0x8d));
    check(output == expected); // Every tag, pause, newline and unrelated byte survives.

    auto message = context(title);
    for (int language : {1, 2, 3, 4, 5, 6, 255}) {
        message.language = static_cast<uint8_t>(language);
        check(!kingdom::text::rewrite(message, output));
        check(output.empty());
    }
    message = context(title);
    message.group = 1;
    check(!kingdom::text::rewrite(message, output));
    message = context(title, 0x18e); // Master Sword is never targeted.
    check(!kingdom::text::rewrite(message, output));
    message = context(title);
    message.original_text = nullptr;
    check(!kingdom::text::rewrite(message, output));
    message = context(title);
    message.original_text_size = 65537;
    check(!kingdom::text::rewrite(message, output));
    check(!rewrite(encoded("Master Sword"), output));
    check(!rewrite(encoded("Kingdom Key"), output));

    // A text-looking tag argument must remain byte-for-byte unchanged.
    std::vector<uint8_t> tag{0x1a, 16, 7, 0, 0};
    tag.insert(tag.end(), title.begin(), title.end() - 1);
    auto tagOnly = tag;
    tagOnly.push_back(0);
    check(!rewrite(tagOnly, output));
    auto taggedTitle = tag;
    taggedTitle.insert(taggedTitle.end(), title.begin(), title.end());
    check(rewrite(taggedTitle, output));
    auto expectedTagged = tag;
    const auto renamed = encoded("Kingdom Key");
    expectedTagged.insert(expectedTagged.end(), renamed.begin(), renamed.end());
    check(output == expectedTagged);

    // Truncating any byte of either real message must fail without a partial result.
    for (const auto& sample : {title, pickup, taggedTitle}) {
        for (size_t size = 0; size < sample.size(); ++size) {
            const std::vector<uint8_t> truncated(sample.begin(), sample.begin() + size);
            check(!rewrite(truncated, output));
            check(output.empty());
        }
    }
    auto malformed = title;
    malformed.push_back('x'); // Data after the terminator.
    check(!rewrite(malformed, output));
    malformed = title;
    malformed.back() = 0x1a; // Missing tag length.
    check(!rewrite(malformed, output));
    malformed = title;
    malformed.pop_back();
    malformed.insert(malformed.end(), {0x1a, 4, 0, 0, 0});
    check(!rewrite(malformed, output));
    malformed = title;
    malformed.pop_back();
    malformed.insert(malformed.end(), {0x1a, 6, 0, 0, 0, 0}); // Tag consumes final NUL.
    check(!rewrite(malformed, output));
    malformed = title;
    malformed.pop_back();
    malformed.insert(malformed.end(), {0x1a, 7, 255, 0, 0, 1, 1, 0}); // Invalid color size.
    check(!rewrite(malformed, output));
    check(output.empty());

    // Integration contract: exact registration scopes, stable callback result,
    // rollback on second-registration failure, repeat initialization and shutdown.
    MessageService service{};
    service.override_message_fn = registerMessage;
    service.remove_override = removeMessage;
    int opaqueContext = 0;
    auto* host = reinterpret_cast<ModContext*>(&opaqueContext);
    kingdom::text::System system;
    check(system.initialize(nullptr, &service) == MOD_INVALID_ARGUMENT);
    check(system.initialize(host, nullptr) == MOD_INVALID_ARGUMENT);
    failOnAttempt = 2;
    check(system.initialize(host, &service) == MOD_UNAVAILABLE);
    check(registered.empty() && removals == 1);
    system.shutdown();
    check(removals == 1);
    failOnAttempt = 0;
    check(system.initialize(host, &service) == MOD_OK);
    check(registered.size() == 2);
    for (const auto& [handle, entry] : registered) {
        check(entry.group == 0 && entry.language == MESSAGE_LANGUAGE_ENGLISH);
        check(entry.id == 0x18d || entry.id == 0x8d);
        auto original = context(entry.id == 0x18d ? title : pickup, entry.id);
        MessageTextData result{};
        check(entry.callback(host, &original, &result, entry.data));
        const std::vector<uint8_t> copied(result.text, result.text + result.text_size);
        check(copied == (entry.id == 0x18d ? renamed : expected));
        check(!entry.callback(host, nullptr, &result, entry.data));
        check(!entry.callback(host, &original, nullptr, entry.data));
        check(!entry.callback(host, &original, &result, nullptr));
    }
    check(system.initialize(host, &service) == MOD_OK);
    check(registered.size() == 2 && removals == 3);
    system.shutdown();
    check(registered.empty() && removals == 5);
    system.shutdown();
    check(removals == 5);
    std::cout << "PASS: " << checks << " text and MessageService lifecycle checks\n";
}
