// From a C++20 developer prompt, with assertions enabled:
// cl /nologo /EHsc /std:c++20 /W4 /I <dusklight>/sdk/include tools/ui_regression.cpp /Fe:ui_regression.exe
// .\ui_regression.exe
// This exercises v0.2.3g texture service ownership and pane behavior without a game process.
#include "../src/keyblade_ui.hpp"

#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <set>

namespace {
std::array<unsigned char, 34> pngHeader = {
    137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 'I', 'H', 'D', 'R',
    0, 0, 1, 0, 0, 0, 1, 0, 8, 6, 0, 0, 0, 0, 0, 0, 0, 0
};
int allocations = 0;
int registrations = 0;
bool failLoad = false;
bool failRegister = false;
bool failedHandle = false;
TextureReplacementHandle nextHandle = 1;
std::set<TextureReplacementHandle> handles;

// Same protected member/access shape as J2DPicture, without a game renderer.
// setMirror(X) models the verified native full-bound Collection UV operation.
struct Coordinate { int16_t x, y; };
class Picture {
protected:
    Coordinate field_0x10a[4]{};
public:
    using UV = std::array<std::array<int16_t, 2>, 4>;
    UV snapshot() const {
        UV result{};
        for (size_t i = 0; i < 4; ++i) result[i] = {field_0x10a[i].x, field_0x10a[i].y};
        return result;
    }
    void set(const UV& uv) {
        for (size_t i = 0; i < 4; ++i) field_0x10a[i] = {uv[i][0], uv[i][1]};
    }
    void setMirror(int mirror) {
        assert(mirror == 2);
        set({{{256, 0}, {0, 0}, {256, 256}, {0, 256}}});
    }
};
struct Pane {
    float mRotateZ = 76.0f;
    float centerX = 24, centerY = 24, scaleX = 2.3f, scaleY = 2.3f, x = 17, y = 25;
    int matricesUpdated = 0;
    void calcMtx() { ++matricesUpdated; }
};

ModResult load(ModContext*, const char* path, ResourceBuffer* buffer) {
    assert(std::string_view(path) == kingdom::ui::kIconBundlePath + 4);
    if (failLoad) return MOD_UNAVAILABLE;
    buffer->size = pngHeader.size();
    buffer->data = std::malloc(buffer->size);
    assert(buffer->data);
    std::memcpy(buffer->data, pngHeader.data(), buffer->size);
    ++allocations;
    return MOD_OK;
}
void freeBuffer(ModContext*, ResourceBuffer* buffer) {
    if (buffer->data) {
        std::free(buffer->data);
        --allocations;
    }
    buffer->data = nullptr;
    buffer->size = 0;
}
ModResult registerFile(ModContext*, const char* path, TextureReplacementHandle* handle) {
    assert(std::string_view(path) == kingdom::ui::kIconBundlePath);
    ++registrations;
    *handle = 0;
    if (!failRegister || failedHandle) {
        *handle = nextHandle++;
        handles.insert(*handle);
    }
    return failRegister ? MOD_ERROR : MOD_OK;
}
ModResult unregisterFile(ModContext*, TextureReplacementHandle handle) {
    assert(handles.erase(handle) == 1);
    return MOD_OK;
}
}

int main() {
    ResourceService resources{};
    resources.load = load;
    resources.free = freeBuffer;
    TextureService textures{};
    textures.register_file = registerFile;
    textures.unregister = unregisterFile;
    unsigned char opaqueContext = 0;
    auto* context = reinterpret_cast<ModContext*>(&opaqueContext);
    kingdom::ui::System icons;
    std::string error;

    icons.shutdown();
    assert(icons.initialize(context, &resources, &textures, error));
    assert(error.empty() && allocations == 0 && handles.size() == 1);
    Picture collection, button;
    const Picture::UV unmirrored{{{0, 0}, {256, 0}, {0, 256}, {256, 256}}};
    const Picture::UV mirrored{{{256, 0}, {0, 0}, {256, 256}, {0, 256}}};
    const Picture::UV customUV{{{18, 5}, {215, 5}, {18, 218}, {215, 218}}};
    collection.set(unmirrored);
    button.set(unmirrored);
    assert(icons.beginCollectionDraw(&collection, 2));
    assert(collection.snapshot() == mirrored && button.snapshot() == unmirrored);
    assert(icons.endCollectionDraw(&collection));
    assert(collection.snapshot() == unmirrored);
    collection.set(customUV);
    assert(icons.beginCollectionDraw(&collection, 2));
    assert(icons.beginCollectionDraw(&collection, 2));
    assert(icons.endCollectionDraw(&collection));
    assert(collection.snapshot() == mirrored);
    assert(icons.endCollectionDraw(&collection));
    assert(collection.snapshot() == customUV);
    assert(!icons.endCollectionDraw(&collection));
    // Excess nesting is balanced without dropping an outer restore frame.
    for (int i = 0; i < 20; ++i)
        assert(icons.beginCollectionDraw(&collection, 2) == (i < 8));
    for (int i = 19; i >= 0; --i)
        assert(icons.endCollectionDraw(&collection) == (i < 8));
    assert(collection.snapshot() == customUV && button.snapshot() == unmirrored);
    assert(!icons.beginCollectionDraw(static_cast<Picture*>(nullptr), 2));
    assert(!icons.endCollectionDraw(static_cast<Picture*>(nullptr)));
    Pane swordButton, otherButton;
    assert(icons.beginHudDraw(&swordButton));
    assert(swordButton.mRotateZ == 0 && otherButton.mRotateZ == 76);
    assert(swordButton.centerX == 24 && swordButton.centerY == 24);
    assert(swordButton.scaleX == 2.3f && swordButton.scaleY == 2.3f);
    assert(swordButton.x == 17 && swordButton.y == 25);
    assert(icons.beginHudDraw(&swordButton));
    assert(icons.endHudDraw(&swordButton) && swordButton.mRotateZ == 0);
    // Native presentation can advance location/scale within draw; restoring
    // only the saved angle must preserve those newer values.
    swordButton.x = 19;
    swordButton.scaleX = 2.5f;
    assert(icons.endHudDraw(&swordButton) && swordButton.mRotateZ == 76);
    assert(swordButton.x == 19 && swordButton.scaleX == 2.5f);
    assert(swordButton.matricesUpdated == 4);
    for (int i = 0; i < 20; ++i)
        assert(icons.beginHudDraw(&swordButton) == (i < 8));
    for (int i = 19; i >= 0; --i)
        assert(icons.endHudDraw(&swordButton) == (i < 8));
    assert(swordButton.mRotateZ == 76);
    assert(!icons.endHudDraw(&swordButton));
    assert(!icons.beginHudDraw(static_cast<Pane*>(nullptr)));
    assert(!icons.endHudDraw(static_cast<Pane*>(nullptr)));
    // Re-enable releases the prior replacement and registers exactly one.
    // v0.2.3g uses the public texture service, with no obsolete touch-source API.
    assert(icons.initialize(context, &resources, &textures, error));
    assert(handles.size() == 1);
    ++pngHeader.back();
    assert(icons.initialize(context, &resources, &textures, error));
    assert(allocations == 0 && handles.size() == 1);
    icons.shutdown();
    icons.shutdown();
    assert(handles.empty());

    failLoad = true;
    assert(!icons.initialize(context, &resources, &textures, error));
    assert(!error.empty() && handles.empty() && allocations == 0);
    failLoad = false;
    auto attempts = registrations;
    pngHeader[0] = 0;
    assert(!icons.initialize(context, &resources, &textures, error));
    assert(registrations == attempts && allocations == 0);
    pngHeader[0] = 137;
    pngHeader[23] = 1;
    assert(!icons.initialize(context, &resources, &textures, error));
    assert(registrations == attempts && allocations == 0);
    pngHeader[23] = 0;
    failRegister = true;
    assert(!icons.initialize(context, &resources, &textures, error));
    assert(handles.empty() && allocations == 0);
    // Defensively reclaim a handle even if an unexpected failing host returns one.
    failedHandle = true;
    assert(!icons.initialize(context, &resources, &textures, error));
    assert(handles.empty() && allocations == 0);
    failRegister = failedHandle = false;
    assert(icons.initialize(context, &resources, &textures, error));
    icons.shutdown();
    assert(handles.empty() && allocations == 0);
    assert(!icons.beginCollectionDraw(&collection, 2));
    assert(!icons.endCollectionDraw(&collection));
    assert(collection.snapshot() == customUV);
    assert(!icons.beginHudDraw(&swordButton));
    assert(!icons.endHudDraw(&swordButton));
    assert(swordButton.mRotateZ == 76);
    std::puts("PASS: UI resource/texture ownership, failure rollback, disable/re-enable, Collection-only UV mirror, HUD-only angle correction, exact restoration and bounded nesting.");
}
