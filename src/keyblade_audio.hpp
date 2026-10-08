#pragma once

// Independent native sound effects for Dusklight 2.0.2. WAV samples are owned
// by AudioResService; JAISe supplies pause, SFX volume, distance and pan. No
// original sound, instrument or music stream is replaced.
#include "mods/svc/audio_res.h"
#include "mods/svc/hook.h"
#include "JSystem/JAudio2/JASBank.h"
#include "JSystem/JAudio2/JASBasicInst.h"
#include "JSystem/JAudio2/JASCriticalSection.h"
#include "JSystem/JAudio2/JAISe.h"
#include "JSystem/JAudio2/JASVoiceBank.h"
#include "JSystem/JAudio2/JASWaveInfo.h"
#include "Z2AudioLib/Z2AudioMgr.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace kingdom::audio {

// Dusklight 2.0.2 implements JASCriticalSection on every native host platform
// as the guard for its recursive audio-thread mutex. Resolve those host
// constructor/destructor symbols at runtime so the mod uses the *same* mutex
// as the game without linking platform-specific C++ ABI symbols into the mod.
// HookService display names are platform-independent; decorated-name fallbacks
// cover constructors/destructors if a symbol manifest reports them ambiguously.
enum class Cue : std::uint8_t { Summon, Dismiss, Hit, Hit2, Hit3, Hit4, Hit5, Hit6, Hit7, Count };
constexpr std::size_t kCueCount=static_cast<std::size_t>(Cue::Count);

class System {
    // JASBank::noteOn asks for an initial wave handle before consulting the
    // SDK's wave map. A non-null sentinel permits our new wave IDs to reach
    // that map. If registration is unavailable, the null info fails silently.
    class Sentinel final : public JASWaveHandle {
    public:
        const JASWaveInfo* getWaveInfo() const override { return nullptr; }
        intptr_t getWavePtr() const override { return 0; }
        const void* getAramBaseAddress() const override { return nullptr; }
    };
    class WaveBank final : public JASWaveBank {
        mutable Sentinel sentinel_;
    public:
        std::array<std::uint16_t,kCueCount> ids{};
        std::array<bool,kCueCount> loaded{};
        WaveBank() : JASWaveBank(AUDIO_WAVE_BANK_SOUND_EFFECTS) {}
        JASWaveHandle* getWaveHandle(u32 id) const override {
            for(std::size_t i=0;i<kCueCount;++i)if(loaded[i]&&ids[i]==id)return &sentinel_;
            return nullptr;
        }
        JASWaveArc* getWaveArc(u32) override { return nullptr; }
        u32 getArcCount() const override { return 0; }
    } waves_;
    class Bank final : public JASBank {
        WaveBank& waves_;
        // Channels retain this host-owned oscillator data, not a DLL-owned
        // callback or envelope, so a short release can finish after unload.
        JASOscillator::Data* oscillator_=const_cast<JASOscillator::Data*>(&JASVoiceBank::sOscData);
    public:
        explicit Bank(WaveBank& waves) : waves_(waves) { assignWaveBank(&waves); }
        bool getInstParam(int program,int,int,JASInstParam* out) const override {
            if(program<0||program>=static_cast<int>(kCueCount)||!waves_.loaded[program])return false;
            *out=JASInstParam{};
            out->mWaveId=waves_.ids[program];
            out->mOscillatorCount=1;
            out->mOscillators=const_cast<JASOscillator::Data**>(&oscillator_);
            out->mDirectRelease=1;
            return true;
        }
        u32 getType() const override { return 0x4b4b4559; } // KKEY
    } bank_{waves_};

    struct Sound {
        AudioWaveHandle wave{};
        AudioSoundTableHandle table{};
        std::uint16_t effect{};
        bool ready{};
        // bank/program; key60/velocity127, gate0 waits for the non-looped wave
        // to finish; end. These are newly authored instructions, not disc data.
        std::array<std::uint8_t,8> sequence{};
    };
    std::array<Sound,kCueCount> sounds_{};
    std::array<JAISoundHandle,8> voices_{};
    ModContext* context_{};
    const AudioResService* service_{};
    int bankSlot_=-1;
    std::size_t nextVoice_{};
    bool enabled_{};
    bool initialized_{};
#if defined(_MSC_VER)
    // MSVC constructors return `this` in the native ABI. The return value is
    // intentionally ignored; the object exists only to enter the host mutex.
    using CriticalEnter=void*(*)(void*);
#else
    // Itanium C++ ABI constructors (Linux/Android/Apple) return void.
    using CriticalEnter=void(*)(void*);
#endif
    using CriticalExit=void(*)(void*);
    CriticalEnter criticalEnter_{};
    CriticalExit criticalExit_{};

    static bool resolveCode(const HookService* hooks,ModContext* context,const char* symbol,void** out) {
        if(!hooks||!context||!symbol||!out)return false;
        HookSymbolFlags flags{};
        return hooks->resolve(context,symbol,out,&flags)==MOD_OK
            &&(*out)!=nullptr&&(flags&HOOK_SYMBOL_CODE)!=0;
    }
    bool resolveAudioLock(const HookService* hooks) {
        void* enter=nullptr;void* leave=nullptr;

        // Preferred path: Dusklight's platform-neutral demangled aliases.
        if(resolveCode(hooks,context_,"JASCriticalSection::JASCriticalSection",&enter)
            &&resolveCode(hooks,context_,"JASCriticalSection::~JASCriticalSection",&leave)) {
            criticalEnter_=reinterpret_cast<CriticalEnter>(enter);
            criticalExit_=reinterpret_cast<CriticalExit>(leave);
            return true;
        }

        // Constructors/destructors can have multiple ABI symbols. If the
        // display alias is ambiguous, use the exact native ABI spelling.
#if defined(_MSC_VER)
        constexpr const char* enterNames[]={"??0JASCriticalSection@@QEAA@XZ"};
        constexpr const char* leaveNames[]={"??1JASCriticalSection@@QEAA@XZ"};
#else
        // Itanium ABI: C1/C2 are complete/base constructors; D1/D2 are the
        // corresponding destructors. Dusklight's class has no virtual bases,
        // so either emitted variant is suitable for this stack guard.
        constexpr const char* enterNames[]={"_ZN18JASCriticalSectionC1Ev","_ZN18JASCriticalSectionC2Ev"};
        constexpr const char* leaveNames[]={"_ZN18JASCriticalSectionD1Ev","_ZN18JASCriticalSectionD2Ev"};
#endif
        for(const char* name:enterNames)if(resolveCode(hooks,context_,name,&enter))break;
        for(const char* name:leaveNames)if(resolveCode(hooks,context_,name,&leave))break;
        if(!enter||!leave)return false;
        criticalEnter_=reinterpret_cast<CriticalEnter>(enter);
        criticalExit_=reinterpret_cast<CriticalExit>(leave);
        return true;
    }

    class AudioLock {
        System& owner_;
        alignas(JASCriticalSection) unsigned char storage_[sizeof(JASCriticalSection)]{};
    public:
        explicit AudioLock(System& owner) : owner_(owner) {
            if(owner_.criticalEnter_)owner_.criticalEnter_(storage_);
        }
        ~AudioLock() { if(owner_.criticalExit_)owner_.criticalExit_(storage_); }
    };

    static u32 soundID(const Sound& sound) {
        return (static_cast<u32>(SE_CATEGORY_PLAYER_SE)<<16)|sound.effect;
    }
    static void stopSound(JAISound* sound) {
        // Call under JASCriticalSection. The host audio driver uses that same
        // recursive mutex; it cannot be reading this sequence while cleared.
        if(!sound)return;
        sound->stop();
        if(JAISe* se=sound->asSe()) {
            JASTrack* track=se->getTrack();
            if(track->getStatus()==JASTrack::STATUS_RUN)track->stopSeq();
            track->getSeqCtrl()->init();
            se->inner_.mSeqData.set(nullptr,0);
        }
    }
    static void stopVoice(JAISoundHandle& handle) {
        if(handle)stopSound(handle.getSound());
        handle.releaseSound();
    }
    void stopOwnedSounds() {
        for(auto& voice:voices_)stopVoice(voice);
        // A native scene stop detaches handles before its audio thread has
        // finished the track. Include those pending sounds during cleanup,
        // rather than assuming an empty handle proves all readers are gone.
        auto* audio=Z2AudioMgr::getInterface();
        if(!audio)return;
        const auto* list=audio->mSoundMgr.getSeMgr()->getCategory(SE_CATEGORY_PLAYER_SE)->getSeList();
        for(auto* node=list->getFirst();node;node=node->getNext()) {
            JAISe* se=node->getObject();
            const u32 id=se->getID();
            for(const Sound& sound:sounds_)if(sound.ready&&id==soundID(sound)) {
                stopSound(se);break;
            }
        }
    }
public:
    System()=default;
    System(const System&)=delete;
    System& operator=(const System&)=delete;

    // Call once during mod_initialize, before the host applies its service
    // lifecycle. The 2.0.x BST service synchronizes additions reliably at
    // this boundary. Files must be mono 16-bit PCM WAV, bundled under res/.
    // Returns how many cues loaded; a missing cue stays unavailable.
    unsigned initialize(ModContext* context,const AudioResService* service,const HookService* hooks) {
        if(initialized_||!context||!service||!hooks)return 0;
        initialized_=true;context_=context;service_=service;
        // JASCriticalSection itself is not part of the mod import surface, so
        // resolve the host implementation through HookService. This preserves
        // the audio driver's synchronization on every supported native ABI.
        if(!resolveAudioLock(hooks))return 0;
        {
            AudioLock lock(*this);
            // TP's own instrument banks occupy low indices. Claim a vacant
            // high slot and release only our own pointer on shutdown.
            for(int candidate=255;candidate>=240;--candidate) {
                if(!JASTrack::sDefaultBankTable.getBank(candidate)) {bankSlot_=candidate;break;}
            }
        }
        if(bankSlot_<0)return 0;
        constexpr const char* paths[kCueCount]={
            "res/kingdom_key_summon.wav","res/kingdom_key_dismiss.wav","res/kingdom_key_hit.wav",
            "res/kingdom_key_hit_02.wav","res/kingdom_key_hit_03.wav","res/kingdom_key_hit_04.wav",
            "res/kingdom_key_hit_05.wav","res/kingdom_key_hit_06.wav","res/kingdom_key_hit_07.wav"};
        unsigned loaded=0;
        for(std::size_t i=0;i<kCueCount;++i) {
            Sound& sound=sounds_[i];
            if(service_->add_wave(context_,AUDIO_WAVE_BANK_SOUND_EFFECTS,paths[i],nullptr,
                                  &sound.wave,&waves_.ids[i])!=MOD_OK)continue;
            AudioSoundTableEffectInfo info=*service_->default_effect_info;
            info.priority=160;info.volume=1.0f;info.pitch=1.0f;
            info.random_volume=0;info.random_pitch=0;info.doppler_power=0;
            if(service_->add_sound_table_effect(context_,SE_CATEGORY_PLAYER_SE,&info,
                                                &sound.table,&sound.effect)!=MOD_OK) {
                service_->remove_wave(context_,sound.wave);sound.wave=0;continue;
            }
            sound.sequence={0xe1,static_cast<std::uint8_t>(bankSlot_),static_cast<std::uint8_t>(i),
                            60,0,127,0,0xff};
            sound.ready=true;waves_.loaded[i]=true;++loaded;
        }
        if(loaded) {
            AudioLock lock(*this);
            // Initialization runs on the mod lifecycle thread, but recheck so
            // another claimant is never overwritten.
            if(JASTrack::sDefaultBankTable.getBank(bankSlot_))return 0;
            JASTrack::sDefaultBankTable.registBank(bankSlot_,&bank_);
            enabled_=true;
        }
        return loaded;
    }
    bool ready(Cue cue) const {
        const auto i=static_cast<std::size_t>(cue);
        return enabled_&&i<kCueCount&&sounds_[i].ready;
    }

    // Used by a PRE hook on JAISe::prepare_getSeqData_. When true, set that
    // function's bool return to true and skip its original implementation.
    // Other sounds continue through their original data manager unchanged.
    bool prepareSequence(JAISe* se) {
        if(!enabled_||!se)return false;
        const u32 id=se->getID();
        for(const Sound& sound:sounds_)if(sound.ready&&id==soundID(sound)) {
            se->inner_.mSeqData.set(sound.sequence.data(),0);
            return true;
        }
        return false;
    }

    // Call once per confirmed event, from simulation. nullptr is nonpositional;
    // a point uses the ordinary native SFX distance/panning calculation. The
    // engine copies the point into its audible object, not a borrowed pointer.
    bool play(Cue cue,const Vec* position=nullptr,float volume=1.0f,float pitch=1.0f) {
        if(!ready(cue))return false;
        auto* audio=Z2AudioMgr::getInterface(); // imported host data, not a local template singleton
        if(!audio||audio->isResetting())return false;
        if(!std::isfinite(volume)||!std::isfinite(pitch))return false;
        AudioLock lock(*this);
        if(JASTrack::sDefaultBankTable.getBank(bankSlot_)!=&bank_)return false;
        JAISoundHandle* handle=nullptr;
        for(auto& voice:voices_)if(!voice) {handle=&voice;break;}
        if(!handle) {handle=&voices_[nextVoice_++%voices_.size()];stopVoice(*handle);}
        JGeometry::TVec3<f32> point;
        if(position)point.set(position->x,position->y,position->z);
        const Sound& sound=sounds_[static_cast<std::size_t>(cue)];
        if(!audio->mSoundMgr.startSound(JAISoundID(soundID(sound)),handle,position?&point:nullptr))return false;
        if(!*handle)return false;
        (*handle)->getAuxiliary().moveVolume(std::clamp(volume,0.0f,1.0f),0);
        (*handle)->getAuxiliary().movePitch(std::clamp(pitch,0.5f,2.0f),0);
        return true;
    }
    void stopAll() {
        AudioLock lock(*this);
        stopOwnedSounds();
    }
    // Call BEFORE removing the prepare hook or unloading the DLL. No native
    // reader, sound handle, bank slot or sample callback then refers into it.
    void shutdown() {
        {
            AudioLock lock(*this);
            enabled_=false;
            stopOwnedSounds();
            if(bankSlot_>=0&&JASTrack::sDefaultBankTable.getBank(bankSlot_)==&bank_)
                JASTrack::sDefaultBankTable.registBank(bankSlot_,nullptr);
        }
        if(service_)for(auto& sound:sounds_)if(sound.wave) {
            service_->remove_wave(context_,sound.wave);sound.wave=0;
        }
        // Sound-table additions are owned and removed by the host's mod-detach
        // lifecycle. Do not call remove_sound_table here; the host owns those
        // additions and removes them when the mod detaches.
        for(auto& sound:sounds_)sound.ready=false;
        waves_.loaded.fill(false);bankSlot_=-1;
    }
};

} // namespace kingdom::audio
