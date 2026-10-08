#pragma once
// Character-only replacement of native human Link vocal sequences. Original
// sound IDs, handles, metadata, lifetimes, positional/reverb settings and
// gameplay/cutscene dispatch remain in the host. No weapon audio is modified.
#include "character_voice_mapping.hpp"
#include "mods/svc/audio_res.h"
#include "mods/svc/hook.h"
#include "JSystem/JAudio2/JASBank.h"
#include "JSystem/JAudio2/JASBasicInst.h"
#include "JSystem/JAudio2/JASCriticalSection.h"
#include "JSystem/JAudio2/JAISe.h"
#include "JSystem/JAudio2/JASVoiceBank.h"
#include "JSystem/JAudio2/JASWaveInfo.h"
#include "Z2AudioLib/Z2AudioMgr.h"
#include <atomic>
#include <cmath>
#include <span>

namespace kingdom::character_voice {
class System {
    class Sentinel final : public JASWaveHandle {
    public:
        const JASWaveInfo* getWaveInfo() const override {return nullptr;}
        intptr_t getWavePtr() const override {return 0;}
        const void* getAramBaseAddress() const override {return nullptr;}
    };
    class WaveBank final : public JASWaveBank {
        mutable Sentinel sentinel_;
    public:
        std::array<std::uint16_t,kMaximumClips> ids{};
        std::array<bool,kMaximumClips> loaded{};
        std::array<float,kMaximumClips> volumes{};
        WaveBank():JASWaveBank(AUDIO_WAVE_BANK_SOUND_EFFECTS) {}
        JASWaveHandle* getWaveHandle(u32 id) const override {
            for(std::size_t i=0;i<ids.size();++i)
                if(loaded[i]&&ids[i]==id)return &sentinel_;
            return nullptr;
        }
        JASWaveArc* getWaveArc(u32) override {return nullptr;}
        u32 getArcCount() const override {return 0;}
    } waves_;
    class Bank final : public JASBank {
        WaveBank& waves_;
        // This host-owned envelope may outlive a short native release; no
        // retained channel envelope or DSP callback points into the mod DLL.
        JASOscillator::Data* oscillator_=const_cast<JASOscillator::Data*>(&JASVoiceBank::sOscData);
    public:
        explicit Bank(WaveBank& waves):waves_(waves) {assignWaveBank(&waves);}
        bool getInstParam(int program,int,int,JASInstParam* out) const override {
            if(program<0||program>=static_cast<int>(kMaximumClips)||!waves_.loaded[program])return false;
            *out=JASInstParam{};
            out->mWaveId=waves_.ids[program];
            out->mVolume=waves_.volumes[program];
            out->mOscillatorCount=1;
            out->mOscillators=const_cast<JASOscillator::Data**>(&oscillator_);
            out->mDirectRelease=1;
            return true;
        }
        u32 getType() const override {return 0x58494f4e;} // XION
    } bank_{waves_};
    struct Clip {
        AudioWaveHandle wave{};
        Group group=Group::Count;
        std::array<std::uint8_t,8> sequence{};
    };
    std::array<Clip,kMaximumClips> clips_{};
    std::array<std::uint32_t,kGroupCount> next_{};
    ModContext* context_{};
    const AudioResService* service_{};
    int bankSlot_=-1;
    unsigned loadedCount_{};
    bool registered_{};
    std::atomic<bool> ready_{false};
    std::atomic<bool> enabled_{false};
#if defined(_MSC_VER)
    using CriticalEnter=void*(*)(void*);
#else
    using CriticalEnter=void(*)(void*);
#endif
    using CriticalExit=void(*)(void*);
    CriticalEnter criticalEnter_{};
    CriticalExit criticalExit_{};

    static bool resolveCode(const HookService* hooks,ModContext* context,const char* symbol,void** out) {
        HookSymbolFlags flags{};
        void* candidate=nullptr;
        if(hooks->resolve(context,symbol,&candidate,&flags)!=MOD_OK
            ||!candidate||(flags&HOOK_SYMBOL_CODE)==0)return false;
        *out=candidate;
        return true;
    }
    bool resolveAudioLock(const HookService* hooks) {
        void* enter=nullptr;void* leave=nullptr;
        if(resolveCode(hooks,context_,"JASCriticalSection::JASCriticalSection",&enter)
            &&resolveCode(hooks,context_,"JASCriticalSection::~JASCriticalSection",&leave)) {
            criticalEnter_=reinterpret_cast<CriticalEnter>(enter);
            criticalExit_=reinterpret_cast<CriticalExit>(leave);
            return true;
        }
#if defined(_MSC_VER)
        constexpr const char* enterNames[]={"??0JASCriticalSection@@QEAA@XZ"};
        constexpr const char* leaveNames[]={"??1JASCriticalSection@@QEAA@XZ"};
#else
        constexpr const char* enterNames[]={"_ZN18JASCriticalSectionC1Ev","_ZN18JASCriticalSectionC2Ev"};
        constexpr const char* leaveNames[]={"_ZN18JASCriticalSectionD1Ev","_ZN18JASCriticalSectionD2Ev"};
#endif
        for(const auto* name:enterNames)if(resolveCode(hooks,context_,name,&enter))break;
        for(const auto* name:leaveNames)if(resolveCode(hooks,context_,name,&leave))break;
        if(!enter||!leave)return false;
        criticalEnter_=reinterpret_cast<CriticalEnter>(enter);
        criticalExit_=reinterpret_cast<CriticalExit>(leave);
        return true;
    }
    class AudioLock {
        System& owner_;
        alignas(JASCriticalSection) unsigned char storage_[sizeof(JASCriticalSection)]{};
    public:
        explicit AudioLock(System& owner):owner_(owner) {owner_.criticalEnter_(storage_);}
        ~AudioLock() {owner_.criticalExit_(storage_);}
    };
    bool ownsSequence(const void* data) const noexcept {
        if(!data)return false;
        for(const auto& clip:clips_)if(clip.wave&&data==clip.sequence.data())return true;
        return false;
    }
    // Caller holds the host audio mutex. Sound IDs deliberately remain native,
    // so ownership is the exact retained sequence pointer, never an ID range.
    void stopOwnedSoundsLocked() {
        if(!registered_)return;
        auto* audio=Z2AudioMgr::getInterface();
        if(!audio)return;
        const auto* list=audio->mSoundMgr.getSeMgr()->getCategory(SE_CATEGORY_PLAYER_VOICE)->getSeList();
        for(auto* node=list->getFirst();node;) {
            // stop() may unlink the current sound. Capture next before touching it.
            auto* next=node->getNext();
            JAISe* se=node->getObject();
            if(se&&se->getSeqData()&&ownsSequence(se->getSeqData()->mBase)) {
                // Clear the mod-owned sequence while the native audio mutex is
                // held. Do not dereference a track after se->stop(), because
                // native stop may detach/recycle it immediately.
                if(auto* track=se->getTrack()) {
                    if(track->getStatus()==JASTrack::STATUS_RUN)track->stopSeq();
                    if(auto* ctrl=track->getSeqCtrl())ctrl->init();
                }
                se->inner_.mSeqData.set(nullptr,0);
                se->stop();
            }
            node=next;
        }
    }
public:
    System()=default;
    System(const System&)=delete;
    System& operator=(const System&)=delete;

    // One-time lifecycle loading, independent of the selected character.
    // Missing samples/groups retain native Link audio; the character renderer
    // never depends on successful audio initialization. Source WAVs are mono
    // PCM16. Caller owns only the definitions during this call, not playback.
    unsigned initialize(ModContext* context,const AudioResService* service,const HookService* hooks,
                        std::span<const ClipDefinition> definitions) {
        if(context_||!context||!service||!hooks||definitions.empty()
            ||definitions.size()>kMaximumClips)return 0;
        context_=context;service_=service;
        struct Rollback {
            System& system;bool committed=false;
            ~Rollback() {if(!committed)system.shutdown();}
        } rollback{*this};
        if(!resolveAudioLock(hooks))return 0;
        {
            AudioLock lock(*this);
            for(int i=255;i>=240;--i)
                if(!JASTrack::sDefaultBankTable.getBank(i)) {bankSlot_=i;break;}
        }
        if(bankSlot_<0)return 0;
        for(std::size_t i=0;i<definitions.size();++i) {
            const auto& definition=definitions[i];
            if(!definition.path||definition.group>=Group::Count
                ||!std::isfinite(definition.sourceVolume)||definition.sourceVolume<=0
                ||definition.sourceVolume>4.0f)continue;
            auto& clip=clips_[i];
            if(service_->add_wave(context_,AUDIO_WAVE_BANK_SOUND_EFFECTS,definition.path,nullptr,
                &clip.wave,&waves_.ids[i])!=MOD_OK)continue;
            clip.group=definition.group;
            // Same minimal native sequence strategy as the independent weapon
            // helper: bank/program, key60, velocity127, wait for sample, end.
            clip.sequence={0xe1,static_cast<std::uint8_t>(bankSlot_),static_cast<std::uint8_t>(i),
                           60,0,127,0,0xff};
            waves_.volumes[i]=definition.sourceVolume;
            waves_.loaded[i]=true;++loadedCount_;
        }
        if(!loadedCount_)return 0;
        {
            AudioLock lock(*this);
            if(JASTrack::sDefaultBankTable.getBank(bankSlot_))return 0;
            JASTrack::sDefaultBankTable.registBank(bankSlot_,&bank_);
            registered_=true;
            ready_.store(true);
        }
        rollback.committed=true;
        return loadedCount_;
    }
    bool ready() const noexcept {return ready_.load();}
    bool enabled() const noexcept {return enabled_.load();}
    void setEnabled(bool value) {
        if(!criticalEnter_||!criticalExit_) {enabled_.store(false);return;}
        AudioLock lock(*this);
        enabled_.store(value&&registered_&&ready_.load());
        if(!enabled_.load()) {stopOwnedSoundsLocked();next_.fill(0);}
    }
    // Invoke from the shared prepare_getSeqData_ PRE hook. On true, return
    // native bool true and skip original. Everything else passes through.
    bool prepareSequence(JAISe* se) {
        if(!se||!enabled_.load())return false;
        const Group group=groupFor(static_cast<std::uint32_t>(se->getID()));
        if(group==Group::Count)return false;
        AudioLock lock(*this);
        if(!enabled_.load()||!registered_)return false;
        std::array<std::size_t,kMaximumClips> candidates{};
        std::size_t count=0;
        for(std::size_t i=0;i<clips_.size();++i)
            if(clips_[i].wave&&clips_[i].group==group)candidates[count++]=i;
        if(!count)return false;
        const auto index=candidates[next_[static_cast<std::size_t>(group)]++%count];
        se->inner_.mSeqData.set(clips_[index].sequence.data(),0);
        return true;
    }
    void stopAll() {
        // Failed lock resolution must never traverse the native audio list.
        if(!criticalEnter_||!criticalExit_)return;
        AudioLock lock(*this);stopOwnedSoundsLocked();
    }
    void shutdown() {
        enabled_.store(false);ready_.store(false);
        if(criticalEnter_&&criticalExit_) {
            AudioLock lock(*this);
            stopOwnedSoundsLocked();
            if(registered_&&bankSlot_>=0&&JASTrack::sDefaultBankTable.getBank(bankSlot_)==&bank_)
                JASTrack::sDefaultBankTable.registBank(bankSlot_,nullptr);
            registered_=false;
        }
        if(service_)for(auto& clip:clips_)if(clip.wave) {
            service_->remove_wave(context_,clip.wave);clip.wave=0;
        }
        clips_={};waves_.loaded.fill(false);waves_.ids.fill(0);waves_.volumes.fill(0);next_.fill(0);
        loadedCount_=0;bankSlot_=-1;context_=nullptr;service_=nullptr;
        // Lock function pointers remain valid host addresses; retaining them
        // makes repeated shutdown harmless without resolving services again.
    }
};
} // namespace kingdom::character_voice
