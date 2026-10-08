#pragma once

namespace kingdom {

// Cosmetic state only. The game remains responsible for equipping the item,
// attack eligibility, damage and collision. Called once per 30 Hz game tick.
class KeybladePresence {
public:
    enum class Phase { Hidden, Appearing, Held, Disappearing };
    enum class Event { None, Summon, Dismiss };
    // w_so010_Wep specifies .22 s visible delay and .06 s dither transition.
    // The 30 Hz host quantizes these to seven and two simulation ticks.
    static constexpr int visibleDelayTicks = 7;
    static constexpr int fadeTicks = 2;
    static constexpr int transitionTicks = visibleDelayTicks + fadeTicks;

    void synchronize(bool held) {
        initialized_ = true;
        held_ = held;
        phase_ = held ? Phase::Held : Phase::Hidden;
        age_ = transitionTicks;
    }
    void reset() { *this = KeybladePresence{}; }
    Event tick(bool held) {
        if (!initialized_) { synchronize(held); return Event::None; }
        if (held != held_) {
            held_ = held;
            age_ = 0;
            phase_ = held ? Phase::Appearing : Phase::Disappearing;
            return held ? Event::Summon : Event::Dismiss;
        }
        if (age_ < transitionTicks && ++age_ == transitionTicks)
            phase_ = held ? Phase::Held : Phase::Hidden;
        return Event::None;
    }
    bool visible() const {
        if(phase_==Phase::Appearing)return age_>=visibleDelayTicks;
        if(phase_==Phase::Disappearing)return age_<visibleDelayTicks;
        return phase_==Phase::Held;
    }
    bool disappearing() const { return phase_ == Phase::Disappearing; }
    bool held() const { return held_; }
    Phase phase() const { return phase_; }
    float emission() const {
        if(!visible())return 0.0f;
        if (phase_ == Phase::Appearing)
            return 1.0f - static_cast<float>(age_-visibleDelayTicks) / fadeTicks;
        if (phase_ == Phase::Disappearing)
            return age_ < visibleDelayTicks-fadeTicks ? 0.0f
                : static_cast<float>(age_-(visibleDelayTicks-fadeTicks)+1)/fadeTicks;
        return 0.0f;
    }

private:
    bool initialized_ = false;
    bool held_ = false;
    Phase phase_ = Phase::Hidden;
    int age_ = transitionTicks;
};
}
