#pragma once

#include "mods/svc/config.h"
#include "mods/svc/ui.h"
#include "character_selection.hpp"
#include "keyblade_selection.hpp"

#include <string>
#include <string_view>

namespace kingdom::selections {

struct Selection {
    KeybladeSelection keyblade = KeybladeSelection::KingdomKey;
    CharacterSelection character = CharacterSelection::None;
    bool operator==(const Selection&) const = default;
};

// All methods/callbacks run on the game thread. This module owns settings,
// never actors, model pointers, rendering state, audio, or asset registrations.
// Consume pending requests in mod_update and activate each feature separately.
class System {
    ModContext* context_ = nullptr;
    const ConfigService* config_ = nullptr;
    const UiService* ui_ = nullptr;
    ConfigVarHandle keyblade_ = 0, character_ = 0;
    ConfigSubscriptionHandle keybladeSubscription_ = 0, characterSubscription_ = 0;
    UiElementHandle statusRow_ = 0;
    Selection requested_{};
    std::string status_ = "Choose a Keyblade and a character independently.";
    bool ready_ = false, pending_ = false, statusDirty_ = true;

    static void changed(ModContext*, ConfigVarHandle var, const ConfigVarValue* value,
                        const ConfigVarValue*, void* user) {
        auto& self = *static_cast<System*>(user);
        if (!self.ready_ || !value || value->struct_size < sizeof(ConfigVarValue) ||
            value->type != CONFIG_VAR_INT) return;
        const auto previous = self.requested_;
        bool valid = true;
        if (var == self.keyblade_) {
            valid = validKeybladeSelection(value->int_value);
            self.requested_.keyblade = keybladeSelectionFromId(value->int_value);
        } else if (var == self.character_) {
            valid = validCharacterSelection(value->int_value);
            self.requested_.character = characterSelectionFromId(value->int_value);
        } else {
            return;
        }
        // ConfigService accepts arbitrary integers (including external cvar
        // writes). Unknown choices safely mean None, and the persisted dropdown
        // value is corrected too. The service suppresses same-var re-notification.
        if (!valid) self.config_->set_int(self.context_, var, 0);
        self.pending_ |= self.requested_ != previous;
    }

    static ModResult buildPanel(ModContext*, UiElementHandle pane, void* user,
                                ModError* error) {
        auto& self = *static_cast<System*>(user);
        self.statusRow_ = 0; // Rebuild invalidates every previous element handle.
        if (!self.ready_) return MOD_OK;
        auto result = self.ui_->pane_add_section(self.context_, pane, "Customization");
        if (result != MOD_OK)
            return mods::set_error(error, result, "Could not build the customization panel.");

        UiControlDesc control = UI_CONTROL_DESC_INIT;
        // SELECT requires a window's paired help pane. DROPDOWN also works in
        // the ordinary Mods detail panel and uses its native controller/touch UI.
        control.kind = UI_CONTROL_DROPDOWN;
        control.binding = UI_BINDING_CONFIG_VAR;
        control.label = "Keyblade";
        control.config_var = self.keyblade_;
        control.options = kKeybladeOptions;
        control.option_count = sizeof(kKeybladeOptions) / sizeof(*kKeybladeOptions);
        control.tooltip = "None keeps the game's original weapon.";
        result = self.ui_->pane_add_control(self.context_, pane, &control, nullptr);
        if (result != MOD_OK)
            return mods::set_error(error, result, "Could not build the Keyblade choice.");

        control.label = "Character";
        control.config_var = self.character_;
        control.options = kCharacterOptions;
        control.option_count = sizeof(kCharacterOptions) / sizeof(*kCharacterOptions);
        control.tooltip = "None keeps the game's original character.";
        result = self.ui_->pane_add_control(self.context_, pane, &control, nullptr);
        if (result != MOD_OK)
            return mods::set_error(error, result, "Could not build the character choice.");

        result = self.ui_->pane_add_text(self.context_, pane, self.status_.c_str(), &self.statusRow_);
        self.statusDirty_ = result != MOD_OK;
        return result == MOD_OK ? MOD_OK :
            mods::set_error(error, result, "Could not build the customization status.");
    }

    static ModResult updatePanel(ModContext*, void* user, ModError*) {
        auto& self = *static_cast<System*>(user);
        if (self.ready_ && self.statusDirty_ && self.statusRow_) {
            const auto result = self.ui_->elem_set_text(
                self.context_, self.statusRow_, self.status_.c_str());
            if (result == MOD_OK) self.statusDirty_ = false;
            else self.statusRow_ = 0; // A rebuilt/detached status row is optional.
        }
        return MOD_OK;
    }

    bool fail(std::string& error, const char* message) {
        shutdown();
        error = message;
        return false;
    }

public:
    System() = default;
    System(const System&) = delete;
    System& operator=(const System&) = delete;

    bool initialize(ModContext* context, const ConfigService* config,
                    const UiService* ui, std::string& error) {
        shutdown();
        error.clear();
        if (!context || !config || !ui || !config->register_var ||
            !config->unregister_var || !config->get_int || !config->set_int ||
            !config->subscribe || !config->unsubscribe || !ui->register_mods_panel ||
            !ui->pane_add_section || !ui->pane_add_control || !ui->pane_add_text ||
            !ui->elem_set_text)
            return fail(error, "The public customization services are unavailable.");
        context_ = context;
        config_ = config;
        ui_ = ui;

        ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
        desc.name = "keyblade";
        desc.type = CONFIG_VAR_INT;
        desc.default_int = static_cast<int64_t>(KeybladeSelection::KingdomKey);
        if (config_->register_var(context_, &desc, &keyblade_) != MOD_OK || !keyblade_)
            return fail(error, "Could not register the Keyblade setting.");
        desc.name = "character";
        desc.default_int = static_cast<int64_t>(CharacterSelection::None);
        if (config_->register_var(context_, &desc, &character_) != MOD_OK || !character_)
            return fail(error, "Could not register the character setting.");

        // Saved values and --cvar overrides are applied silently at registration.
        // Always read them before installing subscriptions or reporting a choice.
        int64_t keyblade = 0, character = 0;
        if (config_->get_int(context_, keyblade_, &keyblade) != MOD_OK ||
            config_->get_int(context_, character_, &character) != MOD_OK)
            return fail(error, "Could not read the saved customization settings.");
        if ((!validKeybladeSelection(keyblade) &&
             config_->set_int(context_, keyblade_, 0) != MOD_OK) ||
            (!validCharacterSelection(character) &&
             config_->set_int(context_, character_, 0) != MOD_OK))
            return fail(error, "Could not restore a valid customization choice.");
        requested_ = {keybladeSelectionFromId(keyblade), characterSelectionFromId(character)};

        if (config_->subscribe(context_, keyblade_, changed, this, &keybladeSubscription_) != MOD_OK ||
            !keybladeSubscription_ ||
            config_->subscribe(context_, character_, changed, this, &characterSubscription_) != MOD_OK ||
            !characterSubscription_)
            return fail(error, "Could not watch customization changes.");

        ready_ = true;
        pending_ = true; // Includes the initial state, not just subsequent changes.
        statusDirty_ = true;
        UiModsPanelDesc panel = UI_MODS_PANEL_DESC_INIT;
        panel.build = buildPanel;
        panel.update = updatePanel;
        panel.user_data = this;
        if (ui_->register_mods_panel(context_, &panel) != MOD_OK)
            return fail(error, "Could not register the customization panel.");
        return true;
    }

    Selection requested() const noexcept { return requested_; }

    // Multiple UI changes before an update coalesce into the latest pair.
    // Taking a request does not claim the models were successfully activated.
    bool takePending(Selection& out) noexcept {
        if (!ready_ || !pending_) return false;
        out = requested_;
        pending_ = false;
        return true;
    }

    void setStatus(std::string_view text) {
        if (status_ != text) {
            status_.assign(text);
            statusDirty_ = true;
        }
    }

    // Use for whole-mod teardown or initialization rollback, not when either
    // choice is None. The public UI service has no unregister_mods_panel: its
    // mod-owned panel/callbacks are removed by host detach after mod_shutdown
    // or failed initialization. Keep this System alive for the mod's lifetime.
    void shutdown() noexcept {
        ready_ = false;
        if (config_ && context_) {
            if (characterSubscription_) config_->unsubscribe(context_, characterSubscription_);
            if (keybladeSubscription_) config_->unsubscribe(context_, keybladeSubscription_);
            if (character_) config_->unregister_var(context_, character_);
            if (keyblade_) config_->unregister_var(context_, keyblade_);
        }
        keyblade_ = character_ = 0;
        keybladeSubscription_ = characterSubscription_ = 0;
        context_ = nullptr;
        config_ = nullptr;
        ui_ = nullptr;
        statusRow_ = 0;
        requested_ = {KeybladeSelection::None, CharacterSelection::None};
        pending_ = false;
        statusDirty_ = true;
    }
};

} // namespace kingdom::selections
