#pragma once
// Explicit human Link vocal cues from Dusklight v2.0.3 Z2SeMgr.h. These are
// semantic adaptations, not claims that KH3 has matching TP performance lines.
// IDs remain native; wolf voices, other characters, dialogue and weapon SFX
// are intentionally absent. All animation/cutscene routes converge at JAISe.
#include <array>
#include <cstddef>
#include <cstdint>

namespace kingdom::character_voice {
enum class Group : std::uint8_t { AttackLight, AttackStrong, Jump, Exertion,
    HurtLight, HurtHeavy, Gasp, Scream, Breath, Soft, Count };
constexpr std::size_t kGroupCount=static_cast<std::size_t>(Group::Count);
struct Mapping {std::uint32_t sound;Group group;};
inline constexpr std::array<Mapping,152> kHumanVoiceMap{{
    {0x10000u,Group::AttackLight}, // Z2SE_AL_V_ATTACK_S
    {0x10001u,Group::AttackStrong}, // Z2SE_AL_V_ATTACK_L
    {0x10002u,Group::HurtLight}, // Z2SE_AL_V_DAMAGE_S
    {0x10003u,Group::HurtHeavy}, // Z2SE_AL_V_DAMAGE_L
    {0x10004u,Group::AttackLight}, // Z2SE_AL_V_ATTACK_M
    {0x10005u,Group::Jump}, // Z2SE_AL_V_JUMP_S
    {0x10006u,Group::Jump}, // Z2SE_AL_V_JUMP_L
    {0x10007u,Group::Jump}, // Z2SE_AL_V_BACKTEN
    {0x10008u,Group::HurtLight}, // Z2SE_AL_V_ZENTEN_FAIL
    {0x10009u,Group::HurtLight}, // Z2SE_AL_V_ZENTEN_FAIL_2
    {0x1000Au,Group::Exertion}, // Z2SE_AL_V_GRAB
    {0x1000Bu,Group::Gasp}, // Z2SE_AL_V_FOOT_MISS
    {0x1000Cu,Group::Scream}, // Z2SE_AL_V_FALL
    {0x1000Du,Group::HurtLight}, // Z2SE_AL_V_LANDING_FAIL
    {0x1000Eu,Group::HurtLight}, // Z2SE_AL_V_LANDING_FAIL_2
    {0x1000Fu,Group::Exertion}, // Z2SE_AL_V_LIFTUP_S
    {0x10010u,Group::Exertion}, // Z2SE_AL_V_LIFTUP_L
    {0x10011u,Group::Exertion}, // Z2SE_AL_V_THROW_S
    {0x10012u,Group::Exertion}, // Z2SE_AL_V_THROW_L
    {0x10013u,Group::Exertion}, // Z2SE_AL_V_PUSH_ROCK
    {0x10014u,Group::Breath}, // Z2SE_AL_V_TIRED_S
    {0x10015u,Group::Breath}, // Z2SE_AL_V_TIRED_L
    {0x10016u,Group::Scream}, // Z2SE_AL_V_DIE
    {0x10017u,Group::HurtHeavy}, // Z2SE_AL_V_DIE_SHORT
    {0x10018u,Group::Exertion}, // Z2SE_AL_V_CLIMB
    {0x10019u,Group::Soft}, // Z2SE_AL_V_DRINK
    {0x1001Au,Group::Soft}, // Z2SE_AL_V_DRINK_2
    {0x1001Bu,Group::Exertion}, // Z2SE_AL_V_RUSH_HORSE
    {0x1001Cu,Group::AttackLight}, // Z2SE_AL_V_ATTACK_RUN
    {0x1001Du,Group::AttackLight}, // Z2SE_AL_V_SWING_BOTTLE
    {0x1001Eu,Group::HurtHeavy}, // Z2SE_AL_V_BITTEN_LOOP
    {0x1001Fu,Group::Breath}, // Z2SE_AL_V_D04_WAKE_UP
    {0x10020u,Group::Gasp}, // Z2SE_AL_V_D04_NOTICE
    {0x10021u,Group::Gasp}, // Z2SE_AL_V_D04_SURPRISE_A
    {0x10022u,Group::Gasp}, // Z2SE_AL_V_D04_SURPRISE_B
    {0x10023u,Group::HurtHeavy}, // Z2SE_AL_V_D04_KIDNAPPED
    {0x10024u,Group::Gasp}, // Z2SE_AL_V_D04_SURPRISE_C
    {0x10025u,Group::Jump}, // Z2SE_AL_V_GORONJUMP
    {0x10026u,Group::HurtLight}, // Z2SE_AL_V_DRINK_DAMAGE
    {0x10027u,Group::AttackStrong}, // Z2SE_AL_V_SWING_IB
    {0x10028u,Group::AttackStrong}, // Z2SE_AL_V_THROW_IB
    {0x10029u,Group::Scream}, // Z2SE_AL_V_FALL_MAGMA
    {0x1002Au,Group::Scream}, // Z2SE_AL_V_FALL_QUICKSAND
    {0x1002Bu,Group::AttackLight}, // Z2SE_AL_V_ATTACK_S_FREE
    {0x1002Cu,Group::AttackLight}, // Z2SE_AL_V_ATTACK_M_FREE
    {0x1002Du,Group::AttackStrong}, // Z2SE_AL_V_ATTACK_L_FREE
    {0x10048u,Group::AttackLight}, // Z2SE_AL_V_SUMO_HARITE_ATK
    {0x10049u,Group::HurtLight}, // Z2SE_AL_V_SUMO_HARITE_DMG
    {0x1004Au,Group::AttackLight}, // Z2SE_AL_V_SUMO_TUCKLE_ATK
    {0x1004Bu,Group::Exertion}, // Z2SE_AL_V_SUMO_HOLDED
    {0x1004Cu,Group::Exertion}, // Z2SE_AL_V_SUMO_HOLD_BACK
    {0x1004Du,Group::Exertion}, // Z2SE_AL_V_SUMO_PUSH
    {0x1004Eu,Group::AttackStrong}, // Z2SE_AL_V_SUMO_PUSH_LAST
    {0x1004Fu,Group::HurtHeavy}, // Z2SE_AL_V_SUMO_FALL_LOSE
    {0x10050u,Group::HurtHeavy}, // Z2SE_AL_V_UNDER_WATER
    {0x10051u,Group::AttackStrong}, // Z2SE_AL_V_KAITEN
    {0x10052u,Group::AttackStrong}, // Z2SE_AL_V_KAITEN_FREE
    {0x10053u,Group::HurtHeavy}, // Z2SE_AL_V_DAMAGE_FREEZE
    {0x10054u,Group::HurtHeavy}, // Z2SE_AL_V_DAMAGE_ELEC
    {0x10055u,Group::HurtLight}, // Z2SE_AL_V_DAMAGE_COMIC
    {0x10056u,Group::Jump}, // Z2SE_AL_V_BACKTEN_FREE
    {0x10057u,Group::Jump}, // Z2SE_AL_V_JUMP_HANG
    {0x10058u,Group::Exertion}, // Z2SE_AL_V_CLIMB_WALL
    {0x10059u,Group::Exertion}, // Z2SE_AL_V_THROW_GORON
    {0x1005Au,Group::Exertion}, // Z2SE_AL_V_PULL_CHAIN_FM
    {0x1005Bu,Group::Jump}, // Z2SE_AL_V_DIVING
    {0x1005Cu,Group::HurtLight}, // Z2SE_AL_V_MAGNET_CAUGHT
    {0x1005Du,Group::Breath}, // Z2SE_AL_V_RELAX_A
    {0x1005Eu,Group::Breath}, // Z2SE_AL_V_RELAX_B
    {0x1005Fu,Group::Breath}, // Z2SE_AL_V_RELAX_C
    {0x10060u,Group::Exertion}, // Z2SE_AL_V_TAMING
    {0x10061u,Group::Exertion}, // Z2SE_AL_V_SUMO_SHIKO
    {0x10062u,Group::Exertion}, // Z2SE_AL_V_SUMO_PUSHED_BACK
    {0x10063u,Group::AttackLight}, // Z2SE_AL_V_ATTACK_RUN_FREE
    {0x10064u,Group::Gasp}, // Z2SE_AL_V_D02_LOOKBACK
    {0x10065u,Group::HurtLight}, // Z2SE_AL_V_D02_WRY_FACE
    {0x10066u,Group::Soft}, // Z2SE_AL_V_D04_NOD_SMILE
    {0x10067u,Group::Gasp}, // Z2SE_AL_V_D04_LOOKBACK
    {0x10068u,Group::Gasp}, // Z2SE_AL_V_D04_SURPRISE_D
    {0x10069u,Group::HurtLight}, // Z2SE_AL_V_D04_ATTACKED
    {0x1006Au,Group::Gasp}, // Z2SE_AL_V_D04_SURPRISE_E
    {0x1006Bu,Group::Jump}, // Z2SE_AL_V_D04_DASH
    {0x1006Cu,Group::HurtLight}, // Z2SE_AL_V_D04_SUFFER_A
    {0x1006Du,Group::HurtLight}, // Z2SE_AL_V_D04_SUFFER_B
    {0x1006Eu,Group::HurtHeavy}, // Z2SE_AL_V_D04_SUFFER_C
    {0x1006Fu,Group::HurtHeavy}, // Z2SE_AL_V_D04_FALL_DOWN_A
    {0x10070u,Group::HurtHeavy}, // Z2SE_AL_V_D04_FALL_DOWN_B
    {0x10071u,Group::Scream}, // Z2SE_AL_V_D04_TRANSFORM
    {0x10072u,Group::Gasp}, // Z2SE_AL_V_D11_SURPRISE
    {0x10073u,Group::Exertion}, // Z2SE_AL_V_D15_RUSH_HORSE
    {0x10074u,Group::Gasp}, // Z2SE_AL_V_D15_SURPRISE
    {0x10075u,Group::AttackStrong}, // Z2SE_AL_V_D15_ANGER
    {0x10076u,Group::Jump}, // Z2SE_AL_V_D36_SWAY_JUMP
    {0x10077u,Group::HurtLight}, // Z2SE_AL_V_D36_SWAY_LAND
    {0x10078u,Group::Jump}, // Z2SE_AL_V_D36_JUMP_TO_HORSE
    {0x10079u,Group::Gasp}, // Z2SE_AL_V_D36_LOOSE_BALANCE
    {0x1007Au,Group::Exertion}, // Z2SE_AL_V_D36_HANG_HORSE
    {0x1007Bu,Group::Gasp}, // Z2SE_AL_V_D36_LOOK_BACK
    {0x1007Cu,Group::Soft}, // Z2SE_AL_V_D16_SMILE
    {0x1007Du,Group::Gasp}, // Z2SE_AL_V_D17_LOOK_BACK
    {0x1007Eu,Group::Soft}, // Z2SE_AL_V_D17_NOD
    {0x1007Fu,Group::Soft}, // Z2SE_AL_V_D18_SMILING
    {0x10080u,Group::HurtHeavy}, // Z2SE_AL_V_D18_SHUDDER
    {0x10081u,Group::Soft}, // Z2SE_AL_V_D18_DARK_LAUGH
    {0x10082u,Group::Scream}, // Z2SE_AL_V_D18_SCREAM
    {0x10083u,Group::Gasp}, // Z2SE_AL_V_D18_AWAKE
    {0x10084u,Group::HurtLight}, // Z2SE_AL_V_D19_HIT_SHOULDER
    {0x10085u,Group::Gasp}, // Z2SE_AL_V_D19_WORRIED
    {0x10086u,Group::Gasp}, // Z2SE_AL_V_D19_NOTICE
    {0x10087u,Group::Gasp}, // Z2SE_AL_V_D19_CANT_SAY
    {0x10088u,Group::Gasp}, // Z2SE_AL_V_D35_LOOK_BACK
    {0x10089u,Group::Gasp}, // Z2SE_AL_V_D20_SURPRISE
    {0x1008Au,Group::Gasp}, // Z2SE_AL_V_D20_UPSET
    {0x1008Bu,Group::HurtHeavy}, // Z2SE_AL_V_D20_DAMAGE
    {0x1008Cu,Group::Exertion}, // Z2SE_AL_V_D22_PULLOUT
    {0x1008Du,Group::Breath}, // Z2SE_AL_V_D22_TAKE_A_REST
    {0x1008Eu,Group::Jump}, // Z2SE_AL_V_CANON_JUMP
    {0x1008Fu,Group::Jump}, // Z2SE_AL_V_JUMP_DIVING
    {0x10090u,Group::HurtHeavy}, // Z2SE_AL_V_INSECT_LOOP
    {0x10091u,Group::HurtLight}, // Z2SE_AL_V_GUARD_BROKEN
    {0x10092u,Group::Exertion}, // Z2SE_AL_V_TAME
    {0x10093u,Group::AttackLight}, // Z2SE_AL_V_TATE_OSHI
    {0x10094u,Group::Jump}, // Z2SE_AL_V_TODOME_JUMP
    {0x10095u,Group::Jump}, // Z2SE_AL_V_TODOME_RETURN
    {0x10096u,Group::Jump}, // Z2SE_AL_V_SOTOMO_ROLL
    {0x10097u,Group::AttackLight}, // Z2SE_AL_V_SOTOMO_ATK
    {0x10098u,Group::Jump}, // Z2SE_AL_V_KABUTO_JUMP
    {0x10099u,Group::AttackLight}, // Z2SE_AL_V_KABUTO_ATK
    {0x1009Au,Group::AttackLight}, // Z2SE_AL_V_IAIGIRI
    {0x1009Bu,Group::AttackStrong}, // Z2SE_AL_V_JUMP_ATTACK_L_1
    {0x1009Cu,Group::AttackStrong}, // Z2SE_AL_V_JUMP_ATTACK_L_2
    {0x1009Du,Group::AttackStrong}, // Z2SE_AL_V_KAITENGIRI_L
    {0x1009Eu,Group::Exertion}, // Z2SE_AL_V_OUGI_KAMAE
    {0x1009Fu,Group::Gasp}, // Z2SE_AL_V_ENTRANCE
    {0x100A0u,Group::Gasp}, // Z2SE_AL_V_APPEARANCE
    {0x100A1u,Group::HurtHeavy}, // Z2SE_AL_V_TW_PULL
    {0x100A2u,Group::Jump}, // Z2SE_AL_V_D_MHOP
    {0x100A3u,Group::Exertion}, // Z2SE_AL_V_MSTR_SW_STICK
    {0x100A4u,Group::Exertion}, // Z2SE_AL_V_MSTR_SW_PULLOUT
    {0x100A5u,Group::HurtHeavy}, // Z2SE_AL_V_GET_SWL
    {0x100A6u,Group::Gasp}, // Z2SE_AL_V_D_ODOROKU
    {0x100A7u,Group::Gasp}, // Z2SE_AL_V_D_ASHIMOTO
    {0x100A8u,Group::Soft}, // Z2SE_AL_V_D_UNAZUKU
    {0x100A9u,Group::Scream}, // Z2SE_AL_V_TERRORED
    {0x100ABu,Group::HurtLight}, // Z2SE_AL_V_DAMAGE_ON_HORSE
    {0x100ACu,Group::Exertion}, // Z2SE_AL_V_CLING_HORSE
    {0x100ADu,Group::HurtHeavy}, // Z2SE_AL_V_TRANSFORM
    {0x100C2u,Group::Exertion}, // Z2SE_AL_V_VS_GND_TUBA_A
    {0x100C3u,Group::Exertion}, // Z2SE_AL_V_VS_GND_TUBA_B
    {0x100C4u,Group::Exertion}, // Z2SE_AL_V_VS_GND_TUBA_C
    {0x100C5u,Group::AttackStrong}, // Z2SE_AL_V_VS_GND_TUBA_WIN
    {0x100C6u,Group::HurtHeavy}, // Z2SE_AL_V_VS_GND_TUBA_LOSE
}};
constexpr Group groupFor(std::uint32_t sound) noexcept {
    for(const auto& entry:kHumanVoiceMap)if(entry.sound==sound)return entry.group;
    return Group::Count;
}
struct ClipDefinition {const char* path;Group group;float sourceVolume=1.0f;};
constexpr std::size_t kMaximumClips=64;
} // namespace kingdom::character_voice
