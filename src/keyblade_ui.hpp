#pragma once

#include "mods/svc/resource.h"
#include "mods/svc/texture.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

namespace kingdom::ui {

// The source is the same 48x48 CI8 Ordon icon in itemicon.arc, clctres.arc,
// and clctresR.arc. Hash the original base mip and its referenced palette
// entries 0..126, not the decoded pixels or the unused final palette entry.
inline constexpr char kIconBundlePath[] =
    "res/ui/tex1_48x48_7a16cbeebf26f7d6_3b746898124fa62c_9.png";

namespace detail {
// J2DPicture has a public setMirror() but no coordinate/mirror getter. Form a
// pointer to its inherited protected SDK member using a derived qualifier,
// then apply that base-member pointer to the real base object. No derived
// object, downcast, byte offset, or assumed native object size is involved.
template<class Picture>
struct PictureCoordinates : Picture {
    static auto& get(Picture& picture) noexcept {
        constexpr auto member = &PictureCoordinates::field_0x10a;
        return picture.*member;
    }
};
}

class System {
    ModContext* context_ = nullptr;
    const TextureService* textures_ = nullptr;
    TextureReplacementHandle icon_ = 0;
    struct CollectionFrame {
        const void* picture = nullptr;
        std::array<std::array<int16_t, 2>, 4> coordinates{};
    };
    // Collection drawing is normally nonrecursive. Bound exceptional nesting,
    // balancing ignored calls separately instead of overwriting an outer save.
    std::array<CollectionFrame, 8> collectionFrames_{};
    size_t collectionDepth_ = 0;
    size_t ignoredCollectionDepth_ = 0;
    struct HudFrame {
        const void* pane = nullptr;
        float rotation = 0.0f;
    };
    std::array<HudFrame, 8> hudFrames_{};
    size_t hudDepth_ = 0;
    size_t ignoredHudDepth_ = 0;

    static uint32_t big32(const unsigned char* bytes) noexcept {
        return uint32_t(bytes[0]) << 24 | uint32_t(bytes[1]) << 16 |
               uint32_t(bytes[2]) << 8 | uint32_t(bytes[3]);
    }

public:
    System() = default;
    System(const System&) = delete;
    System& operator=(const System&) = delete;

    bool initialize(ModContext* context, const ResourceService* resources,
                    const TextureService* textures, std::string& error) {
        shutdown();
        error.clear();
        if (!context || !resources || !resources->load || !resources->free ||
            !textures || !textures->register_file || !textures->unregister) {
            error = "Kingdom Key icon services are unavailable.";
            return false;
        }

        struct LoadedResource {
            ModContext* context;
            const ResourceService* service;
            ResourceBuffer buffer = RESOURCE_BUFFER_INIT;
            ~LoadedResource() { service->free(context, &buffer); }
        } image{context, resources};
        // ResourceService resolves within res/, while TextureService and the
        // host's mod:// image provider accept paths relative to the bundle.
        if (resources->load(context, kIconBundlePath + 4, &image.buffer) != MOD_OK) {
            error = "The packaged Kingdom Key icon could not be read.";
            return false;
        }
        const auto* bytes = static_cast<const unsigned char*>(image.buffer.data);
        constexpr unsigned char signature[] = {137, 80, 78, 71, 13, 10, 26, 10};
        if (!bytes || image.buffer.size < 33 || image.buffer.size > 16u * 1024u * 1024u ||
            std::memcmp(bytes, signature, sizeof(signature)) != 0 ||
            big32(bytes + 8) != 13 || std::memcmp(bytes + 12, "IHDR", 4) != 0 ||
            big32(bytes + 16) == 0 || big32(bytes + 16) > 4096 ||
            big32(bytes + 20) != big32(bytes + 16)) {
            error = "The packaged Kingdom Key icon is not a supported square PNG.";
            return false;
        }

        context_ = context;
        textures_ = textures;
        if (textures_->register_file(context_, kIconBundlePath, &icon_) != MOD_OK || !icon_) {
            shutdown();
            error = "The Kingdom Key icon replacement could not be registered.";
            return false;
        }
        // The host retains the bundle and decodes the PNG lazily. This loaded
        // copy is only for validation/revision and is released on return.
        return true;
    }

    // PRE on dMenu_Collect2D_c::_draw: pass only its ken_01 J2DPicture (or null
    // if absent) and J2DMirror_X. This changes pane-local coordinates while
    // leaving the shared texture and every button icon untouched.
    template<class Picture, class Mirror>
    bool beginCollectionDraw(Picture* picture, Mirror horizontalMirror) {
        if (ignoredCollectionDepth_ || collectionDepth_ == collectionFrames_.size()) {
            ++ignoredCollectionDepth_;
            return false;
        }
        auto& frame = collectionFrames_[collectionDepth_++];
        frame = {};
        if (!icon_ || !picture) return false;
        const auto& coordinates = detail::PictureCoordinates<Picture>::get(*picture);
        frame.picture = picture;
        for (size_t i = 0; i < 4; ++i)
            frame.coordinates[i] = {coordinates[i].x, coordinates[i].y};
        picture->setMirror(horizontalMirror);
        return true;
    }

    // POST on the same _draw: search the pane again and restore only if its
    // identity still matches. Restore exact prior UVs, including another mod's
    // crop/mirror, rather than assuming the original MIRROR0 coordinates.
    template<class Picture>
    bool endCollectionDraw(Picture* picture) noexcept {
        if (ignoredCollectionDepth_) {
            --ignoredCollectionDepth_;
            return false;
        }
        if (!collectionDepth_) return false;
        const auto frame = collectionFrames_[--collectionDepth_];
        collectionFrames_[collectionDepth_] = {};
        if (!picture || frame.picture != picture) return false;
        auto& coordinates = detail::PictureCoordinates<Picture>::get(*picture);
        for (size_t i = 0; i < 4; ++i) {
            coordinates[i].x = frame.coordinates[i][0];
            coordinates[i].y = frame.coordinates[i][1];
        }
        return true;
    }

    // PRE/POST on dMeter2Draw_c::draw, passing its mpItemB pane only for item
    // 0x28 (Ordon Sword), otherwise null. Native drawButtonB adds a 76-degree
    // rotation authored for the original sword art; this PNG already has the
    // requested upper-left tip. Touch uses the PNG directly and needs no fix.
    template<class Pane>
    bool beginHudDraw(Pane* pane) {
        if (ignoredHudDepth_ || hudDepth_ == hudFrames_.size()) {
            ++ignoredHudDepth_;
            return false;
        }
        auto& frame = hudFrames_[hudDepth_++];
        frame = {};
        if (!icon_ || !pane) return false;
        frame = {pane, pane->mRotateZ};
        // J2DPane exposes mRotateZ publicly. Change
        // only that named member, preserving its pivot, axis, scale and position.
        pane->mRotateZ = 0.0f;
        pane->calcMtx();
        return true;
    }

    template<class Pane>
    bool endHudDraw(Pane* pane) noexcept {
        if (ignoredHudDepth_) {
            --ignoredHudDepth_;
            return false;
        }
        if (!hudDepth_) return false;
        const auto frame = hudFrames_[--hudDepth_];
        hudFrames_[hudDepth_] = {};
        if (!pane || frame.pane != pane) return false;
        pane->mRotateZ = frame.rotation;
        pane->calcMtx();
        return true;
    }

    void shutdown() noexcept {
        if (icon_ && textures_ && context_)
            textures_->unregister(context_, icon_);
        icon_ = 0;
        textures_ = nullptr;
        context_ = nullptr;
        // Host mod lifecycle changes run at frame boundaries, outside _draw.
        // Every normal POST has already restored its pane and cleared its save.
        collectionFrames_ = {};
        collectionDepth_ = ignoredCollectionDepth_ = 0;
        hudFrames_ = {};
        hudDepth_ = ignoredHudDepth_ = 0;
    }
};

} // namespace kingdom::ui
