#include "../src/keyblade_presence.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using P = kingdom::KeybladePresence;
using E = P::Event;
using S = P::Phase;

void check(const P& p) {
    assert(std::isfinite(p.emission()));
    assert(p.emission() >= 0 && p.emission() <= 1);
    if(p.phase()==S::Hidden)assert(!p.visible());
    if(p.phase()==S::Held)assert(p.visible());
    assert(p.disappearing() == (p.phase() == S::Disappearing));
    if (p.phase() == S::Hidden) assert(!p.held() && p.emission() == 0);
    if (p.phase() == S::Held) assert(p.held() && p.emission() == 0);
    if (p.phase() == S::Appearing) assert(p.held());
    if (p.phase() == S::Disappearing) assert(!p.held());
}

int main() {
    unsigned transitions = 0;
    for (bool baseline : {false, true}) {
        P p;
        assert(p.tick(baseline) == E::None);
        assert(p.phase() == (baseline ? S::Held : S::Hidden));
        check(p);
        for (int k = 0; k < 6000; ++k) {
            assert(p.tick(baseline) == E::None);
            check(p);
        }
        assert(p.tick(!baseline) == (baseline ? E::Dismiss : E::Summon));
        assert(p.visible()==baseline);
        for (int k = 1; k <= P::transitionTicks; ++k) {
            assert(p.tick(!baseline) == E::None);
            assert(p.phase() == (k < P::transitionTicks
                ? (baseline ? S::Disappearing : S::Appearing)
                : (baseline ? S::Hidden : S::Held)));
            assert(p.visible()==(baseline ? k<P::visibleDelayTicks : k>=P::visibleDelayTicks));
            check(p);
        }
        p.reset();
        assert(p.tick(!baseline) == E::None);
        assert(p.phase() == (!baseline ? S::Held : S::Hidden));
    }

    // All 65,536 short state sequences cover rapid cancellation/reversal and
    // verify each native held-state edge triggers exactly one visual event.
    for (unsigned bits = 0; bits < (1u << 16); ++bits) {
        P p;
        bool last = bits & 1;
        assert(p.tick(last) == E::None);
        for (unsigned tick = 1; tick < 16; ++tick) {
            const bool held = (bits >> tick) & 1;
            const E expected = held == last ? E::None : held ? E::Summon : E::Dismiss;
            assert(p.tick(held) == expected);
            if (expected != E::None) ++transitions;
            check(p);
            last = held;
        }
        for (int settle = 0; settle < P::transitionTicks; ++settle)
            assert(p.tick(last) == E::None);
        assert(p.phase() == (last ? S::Held : S::Hidden));
        check(p);
        p.reset();
        assert(p.tick(!last) == E::None);
        check(p);
    }
    std::cout << "PASS: 65536 exhaustive sequences, " << transitions
              << " exact transition events, 12000 stable ticks, KH3 visibility timing,"
                 " rapid reversals, bounded emission, reset suppression.\n";
}
