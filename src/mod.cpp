#include "mods/service.hpp"
#include "mods/svc/hook.hpp"
#include "mods/svc/log.h"
#include "mods/svc/resource.h"
#include "mods/svc/interp.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_mirror.h"
#include "d/d_com_inf_game.h"
#include "d/d_cc_s.h"
#include "d/d_drawlist.h"
#include "d/d_kankyo.h"
#include "d/d_menu_collect.h"
#include "d/d_meter2_info.h"
#include "d/d_meter2_draw.h"
#include "d/d_msg_string_base.h"
#include "d/d_pane_class.h"
#include "JSystem/J3DGraphBase/J3DPacket.h"
#include "JSystem/J3DGraphBase/J3DDrawBuffer.h"
#include "JSystem/J3DGraphBase/J3DShape.h"
#include "chain_physics.hpp"
#include "keyblade_presence.hpp"
#include "kh3_fx.hpp"
#include "shield_gesture.hpp"
#include "keyblade_audio.hpp"
#include "keyblade_ui.hpp"
#include "keyblade_text.hpp"
#include "customization_config.hpp"
#include "xion_renderer.hpp"
#include "character_voice.hpp"
#include "character_voice_clips.hpp"

#include <array>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

DEFINE_MOD();
IMPORT_SERVICE(HookService, svc_hook);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(ResourceService, svc_resource);
IMPORT_SERVICE(InterpService, svc_interp);
IMPORT_SERVICE(AudioResService, svc_audio_res);
IMPORT_SERVICE(TextureService, svc_texture);
IMPORT_SERVICE(MessageService, svc_message);
IMPORT_SERVICE(ConfigService, svc_config);
IMPORT_SERVICE(UiService, svc_ui);
DEFINE_HOOK(&daAlink_c::execute, LinkExecute);
DEFINE_HOOK(&daAlink_c::modelDraw, LinkModelDraw);
DEFINE_HOOK(&daAlink_c::basicModelDraw, LinkBasicModelDraw);
DEFINE_HOOK(&daAlink_c::draw, LinkDraw);
DEFINE_HOOK(&dDlst_shadowReal_c::imageDraw, ShadowImageDraw);
DEFINE_HOOK(&dMirror_packet_c::modelDraw, MirrorModelDraw);
DEFINE_HOOK(&daAlink_c::swordEquip, SwordEquip);
DEFINE_HOOK(&daAlink_c::swordUnequip, SwordUnequip);
DEFINE_HOOK(&daAlink_c::procSwordUnequipSpInit, SwordUnequipSpecial);
DEFINE_HOOK(&mDoExt_MtxCalcAnmBlendTblOld::calc, ShieldArmCalc);
DEFINE_HOOK(&daAlink_c::modelCalc, ShieldModelCalc);
DEFINE_HOOK(&dCcS::ProcAtTgHitmark, SwordHitmark);
DEFINE_HOOK_SYMBOL("JAISe::prepare_getSeqData_", bool(JAISe*), PrepareKeybladeSound);
DEFINE_HOOK(&daAlink_c::seStartSwordCut, SwordDrawSound);
DEFINE_HOOK(&daAlink_c::seStartOnlyReverb, SwordReverbSound);
DEFINE_HOOK(&dMenu_Collect2D_c::_draw, CollectionDraw);
DEFINE_HOOK(&dMeter2Info_getStringKanji, ItemNameString);
DEFINE_HOOK(&dMsgStringBase_c::getStringLocal, ItemDescriptionString);
DEFINE_HOOK(&dMeter2Draw_c::draw, HudDraw);

#include "kingdom_key_runtime.hpp"
using namespace kingdom::keyblade_runtime;
#include "customization_runtime.hpp"
#include "customization_rendering.hpp"


extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error) {
    struct InitializationRollback {
        bool committed=false;
        ~InitializationRollback() {if(!committed)kingdom::customization::shutdown();}
    } rollback;
    ModResult result=MOD_OK;
    result=mods::hook::add_post<ItemNameString>(onItemNameString);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach the Kingdom Key equipment label.");
    result=mods::hook::add_post<ItemDescriptionString>(onItemDescriptionString);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach the Kingdom Key Collection description.");
    result=mods::hook::add_pre<CollectionDraw>(onCollectionDraw);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach the Kingdom Key Collection icon mirror.");
    result=mods::hook::add_post<CollectionDraw>(onCollectionDrawFinished);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach Collection icon restoration.");
    result=mods::hook::add_pre<HudDraw>(onHudDraw);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach the Kingdom Key HUD icon orientation.");
    result=mods::hook::add_post<HudDraw>(onHudDrawFinished);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach HUD icon restoration.");
    // The sequence hook is harmless when custom audio is unavailable: it
    // simply continues to the original game implementation. Install it on all
    // supported platforms and let the audio system resolve the host lock ABI.
    result=mods::hook::add_pre<PrepareKeybladeSound>(kingdom::customization::onPrepareCustomSound);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach custom weapon and character sounds.");
    result=mods::hook::add_pre<SwordDrawSound>(onSwordSound);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach the sword draw sound filter.");
    result=mods::hook::add_pre<SwordReverbSound>(onSwordSound);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach the sword dismissal sound filter.");

    result=mods::hook::add_pre<LinkExecute>(onExecuteStart);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach shield gesture timing.");
    result=mods::hook::replace<ShieldArmCalc>(onShieldArmCalc);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach independent shield arm animation.");
    result=mods::hook::add_pre<ShieldModelCalc>(onShieldModelCalc);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach shield handoff timing.");
    result=mods::hook::add_pre<SwordHitmark>(onSwordHitmark);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach Kingdom Key confirmed-contact tracking.");
    result=mods::hook::add_post<LinkExecute>(onExecute);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach keychain simulation to Link.");
    result=mods::hook::add_pre<LinkModelDraw>(kingdom::customization::onModelDraw);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach the Kingdom Key renderer.");
    result=mods::hook::add_pre<LinkBasicModelDraw>(kingdom::customization::onBasicModelDraw);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach the inventory Kingdom Key renderer.");
    result=mods::hook::add_pre<LinkDraw>(kingdom::customization::onLinkDraw);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach Kingdom Key frame tracking.");
    result=mods::hook::add_post<LinkDraw>(onLinkDrawFinished);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach Kingdom Key light effects.");
    result=mods::hook::add_post<SwordEquip>(onSwordEquip);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach Kingdom Key summoning.");
    result=mods::hook::add_post<SwordUnequip>(onSwordUnequip);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach Kingdom Key dismissal.");
    result=mods::hook::add_pre<SwordUnequipSpecial>(onSwordUnequipSpecial);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach Kingdom Key flourish replacement.");
    result=mods::hook::add_pre<MirrorModelDraw>(kingdom::customization::onMirrorModelDraw);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach Kingdom Key reflections.");
    result=mods::hook::replace<ShadowImageDraw>(kingdom::customization::onShadowImageDraw);
    if(result!=MOD_OK)return mods::set_error(error,result,"Could not attach Kingdom Key shadows.");
    kingdom::customization::initializeAssets();
    std::string configurationError;
    if(!kingdom::customization::settings.initialize(mod_ctx,svc_config,svc_ui,configurationError))
        return mods::set_error(error,MOD_ERROR,configurationError.c_str());
    kingdom::customization::update();
    svc_log->info(mod_ctx,"The Key to Twilight v0.3.0: independent persistent Keyblade and Character selections; v0.2.3g weapon features retained.");
    rollback.committed=true;
    return MOD_OK;
}
MOD_EXPORT ModResult mod_update(ModError*) {
    kingdom::customization::update();
    return MOD_OK;
}
MOD_EXPORT ModResult mod_shutdown(ModError*) {
    kingdom::customization::shutdown();
    return MOD_OK;
}
}
