#include "../src/character_voice_mapping.hpp"
#include <cassert>
#include <iostream>

using namespace kingdom::character_voice;
int main() {
    // Independent native category boundaries from pinned Z2SeMgr.h. Human and
    // wolf IDs are interleaved; a broad category/range replacement is unsafe.
    std::size_t count=0;
    for(std::uint32_t id=0;id<0x110000;++id) {
        const bool human=(id>=0x10000&&id<=0x1002d)
            ||(id>=0x10048&&id<=0x100a9)
            ||(id>=0x100ab&&id<=0x100ad)
            ||(id>=0x100c2&&id<=0x100c6);
        assert((groupFor(id)!=Group::Count)==human);
        if(human)++count;
    }
    assert(count==152&&kHumanVoiceMap.size()==count);
    std::array<std::size_t,kGroupCount> groups{};
    for(std::size_t i=0;i<kHumanVoiceMap.size();++i) {
        const auto& entry=kHumanVoiceMap[i];
        assert(entry.group<Group::Count);
        ++groups[static_cast<std::size_t>(entry.group)];
        for(std::size_t j=0;j<i;++j)assert(entry.sound!=kHumanVoiceMap[j].sound);
    }
    for(auto size:groups)assert(size>0);
    // Free/combat variants must retain the same vocal intent. Cutscene cues
    // use the same mapping as gameplay without touching the dialogue path.
    assert(groupFor(0x10000)==Group::AttackLight);
    assert(groupFor(0x1002b)==Group::AttackLight);
    assert(groupFor(0x10001)==Group::AttackStrong);
    assert(groupFor(0x1002d)==Group::AttackStrong);
    assert(groupFor(0x10020)==Group::Gasp); // D04 notice
    assert(groupFor(0x10082)==Group::Scream); // D18 scream
    assert(groupFor(0x100a8)==Group::Soft); // cutscene nod
    assert(groupFor(0x100c7)==Group::Count); // title wolf howl
    assert(groupFor(0x20000)==Group::Count); // sword summon
    assert(groupFor(0x2000001)==Group::Count); // stream, never dialogue replacement
    std::cout<<"PASS: 152 human vocal cues, all ten semantic groups, wolf/other categories excluded across 1,114,112 IDs.\n";
}
