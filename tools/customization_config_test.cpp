// Assertions must remain enabled. From an MSVC developer prompt:
// cl /nologo /EHsc /std:c++20 /W4 /I <dusklight>/sdk/include tools/customization_config_test.cpp /Fe:customization_config_test.exe
// .\customization_config_test.exe
#include "../src/customization_config.hpp"

#include <cassert>
#include <cstdio>
#include <limits>
#include <map>
#include <vector>

#ifdef NDEBUG
#error "Run customization tests with assertions enabled."
#endif

namespace {
using kingdom::KeybladeSelection;
using kingdom::CharacterSelection;
using kingdom::selections::Selection;
using kingdom::selections::System;
struct Var { std::string name; int64_t value; };
struct Subscription { ConfigVarHandle var; ConfigChangedFn callback; void* user; };
struct Control { std::string label; ConfigVarHandle var; std::vector<std::string> options; };
struct Host {
    std::map<std::string, int64_t> saved;
    std::map<ConfigVarHandle, Var> vars;
    std::map<ConfigSubscriptionHandle, Subscription> subscriptions;
    std::map<UiElementHandle, std::string> text;
    std::vector<Control> controls;
    UiModsPanelDesc panel = UI_MODS_PANEL_DESC_INIT;
    uint64_t next = 1;
    int call = 0, failAt = 0, notifications = 0;
    ConfigVarHandle notifying = 0;
    bool fail() { return ++call == failAt; }
    ConfigVarHandle var(const char* name) const {
        for (const auto& [id, value] : vars) if (value.name == name) return id;
        return 0;
    }
} host;

ModResult registerVar(ModContext*, const ConfigVarDesc* desc, ConfigVarHandle* out) {
    *out = 0;
    if (host.fail()) return MOD_ERROR;
    assert(desc->type == CONFIG_VAR_INT && !host.var(desc->name));
    assert((std::string_view(desc->name) == "keyblade" && desc->default_int == 1) ||
           (std::string_view(desc->name) == "character" && desc->default_int == 0));
    const auto saved = host.saved.find(desc->name);
    const int64_t value = saved == host.saved.end() ? desc->default_int : saved->second;
    *out = host.next++;
    host.vars.emplace(*out, Var{desc->name, value});
    return MOD_OK;
}
ModResult unregisterVar(ModContext*, ConfigVarHandle var) {
    for (const auto& [id, subscription] : host.subscriptions) {
        (void)id;
        assert(subscription.var != var); // Explicit subscriptions go first.
    }
    auto found = host.vars.find(var);
    assert(found != host.vars.end());
    host.saved[found->second.name] = found->second.value;
    host.vars.erase(found);
    return MOD_OK;
}
ModResult getInt(ModContext*, ConfigVarHandle var, int64_t* out) {
    if (host.fail()) return MOD_ERROR;
    *out = host.vars.at(var).value;
    return MOD_OK;
}
ModResult setInt(ModContext* ctx, ConfigVarHandle var, int64_t value) {
    if (host.fail()) return MOD_ERROR;
    auto& slot = host.vars.at(var);
    const auto old = slot.value;
    slot.value = value;
    host.saved[slot.name] = value;
    if (old != value && host.notifying != var) {
        auto subscriptions = host.subscriptions;
        const auto outer = host.notifying;
        host.notifying = var;
        for (const auto& [id, subscription] : subscriptions) {
            (void)id;
            if (subscription.var != var) continue;
            ConfigVarValue current{};
            current.struct_size = sizeof(current);
            current.type = CONFIG_VAR_INT;
            current.int_value = value;
            auto previous = current;
            previous.int_value = old;
            ++host.notifications;
            subscription.callback(ctx, var, &current, &previous, subscription.user);
        }
        host.notifying = outer;
    }
    return MOD_OK;
}
ModResult subscribe(ModContext*, ConfigVarHandle var, ConfigChangedFn callback,
                    void* user, ConfigSubscriptionHandle* out) {
    *out = 0;
    if (host.fail()) return MOD_ERROR;
    assert(host.vars.contains(var));
    *out = host.next++;
    host.subscriptions.emplace(*out, Subscription{var, callback, user});
    return MOD_OK;
}
ModResult unsubscribe(ModContext*, ConfigSubscriptionHandle sub) {
    assert(host.subscriptions.erase(sub) == 1);
    return MOD_OK;
}
ModResult registerPanel(ModContext*, const UiModsPanelDesc* desc) {
    if (host.fail()) return MOD_ERROR;
    host.panel = *desc;
    return MOD_OK;
}
ModResult section(ModContext*, UiElementHandle, const char* label) {
    assert(std::string_view(label) == "Customization");
    return MOD_OK;
}
ModResult control(ModContext*, UiElementHandle, const UiControlDesc* desc, UiElementHandle*) {
    assert(desc->kind == UI_CONTROL_DROPDOWN && desc->binding == UI_BINDING_CONFIG_VAR);
    assert(host.vars.contains(desc->config_var) && desc->option_count == 2);
    assert(desc->get == nullptr && desc->set == nullptr);
    host.controls.push_back({desc->label, desc->config_var,
                             {desc->options[0], desc->options[1]}});
    return MOD_OK;
}
ModResult addText(ModContext*, UiElementHandle, const char* text, UiElementHandle* out) {
    *out = host.next++;
    host.text[*out] = text;
    return MOD_OK;
}
ModResult setText(ModContext*, UiElementHandle row, const char* text) {
    auto found = host.text.find(row);
    if (found == host.text.end()) return MOD_INVALID_ARGUMENT;
    found->second = text;
    return MOD_OK;
}
void build(ModContext* ctx) {
    host.controls.clear();
    host.text.clear();
    ModError error = MOD_ERROR_INIT;
    assert(host.panel.build(ctx, 1000, host.panel.user_data, &error) == MOD_OK);
}
void update(ModContext* ctx) {
    ModError error = MOD_ERROR_INIT;
    assert(host.panel.update(ctx, host.panel.user_data, &error) == MOD_OK);
}
}

int main() {
    ConfigService config{};
    config.register_var = registerVar;
    config.unregister_var = unregisterVar;
    config.get_int = getInt;
    config.set_int = setInt;
    config.subscribe = subscribe;
    config.unsubscribe = unsubscribe;
    UiService ui{};
    ui.register_mods_panel = registerPanel;
    ui.pane_add_section = section;
    ui.pane_add_control = control;
    ui.pane_add_text = addText;
    ui.elem_set_text = setText;
    unsigned char opaque = 0;
    auto* ctx = reinterpret_cast<ModContext*>(&opaque);
    System settings;
    Selection pending;
    std::string error;
    settings.shutdown();
    assert(!settings.takePending(pending));
    assert(settings.initialize(ctx, &config, &ui, error));
    assert(error.empty() && host.notifications == 0);
    assert(settings.takePending(pending));
    assert(pending.keyblade == KeybladeSelection::KingdomKey &&
           pending.character == CharacterSelection::None);
    assert(!settings.takePending(pending));
    build(ctx);
    assert(host.controls.size() == 2);
    assert(host.controls[0].label == "Keyblade" && host.controls[1].label == "Character");
    assert(host.controls[0].options == std::vector<std::string>({"None", "Kingdom Key"}));
    assert(host.controls[1].options == std::vector<std::string>({"None", "Xion"}));
    assert(setInt(ctx, host.var("character"), 1) == MOD_OK);
    assert(settings.takePending(pending));
    assert(pending.keyblade == KeybladeSelection::KingdomKey && pending.character == CharacterSelection::Xion);
    assert(setInt(ctx, host.var("keyblade"), 0) == MOD_OK);
    assert(settings.takePending(pending));
    assert(pending.keyblade == KeybladeSelection::None && pending.character == CharacterSelection::Xion);
    const int notifications = host.notifications;
    assert(setInt(ctx, host.var("keyblade"), 0) == MOD_OK);
    assert(host.notifications == notifications && !settings.takePending(pending));
    assert(setInt(ctx, host.var("keyblade"), 1) == MOD_OK);
    assert(setInt(ctx, host.var("character"), 0) == MOD_OK);
    assert(settings.takePending(pending) && pending == settings.requested());
    assert(pending.keyblade == KeybladeSelection::KingdomKey && pending.character == CharacterSelection::None);
    assert(!settings.takePending(pending));
    settings.setStatus("Xion unavailable; original character active.");
    update(ctx);
    assert(host.text.begin()->second == "Xion unavailable; original character active.");
    host.text.clear(); // Simulate a detached panel before its replacement is built.
    settings.setStatus("Kingdom Key active.");
    update(ctx);
    build(ctx);
    assert(host.text.begin()->second == "Kingdom Key active.");
    assert(setInt(ctx, host.var("character"), 1) == MOD_OK);
    settings.shutdown();
    settings.shutdown();
    assert(host.vars.empty() && host.subscriptions.empty() && !settings.takePending(pending));
    // The host owns panel removal. While detach completes, callbacks safely no-op.
    build(ctx);
    update(ctx);
    assert(host.controls.empty() && host.text.empty());
    assert(settings.initialize(ctx, &config, &ui, error));
    assert(settings.takePending(pending));
    assert(pending.keyblade == KeybladeSelection::KingdomKey && pending.character == CharacterSelection::Xion);
    assert(setInt(ctx, host.var("keyblade"), std::numeric_limits<int64_t>::max()) == MOD_OK);
    assert(settings.takePending(pending));
    assert(pending.keyblade == KeybladeSelection::None && pending.character == CharacterSelection::Xion);
    assert(host.saved["keyblade"] == 0); // Unknown IDs correct to None independently.
    settings.shutdown();
    host = {};
    host.saved = {{"keyblade", -5}, {"character", 800}};
    assert(settings.initialize(ctx, &config, &ui, error));
    assert(settings.takePending(pending));
    assert(pending.keyblade == KeybladeSelection::None && pending.character == CharacterSelection::None);
    assert(host.saved["keyblade"] == 0 && host.saved["character"] == 0);
    settings.shutdown();
    // Inject failure at every state-acquiring/read step, with and without the
    // extra writes needed to repair unknown persisted choices.
    for (bool invalidSaved : {false, true}) {
        for (int failure = 1; failure <= (invalidSaved ? 9 : 7); ++failure) {
            host = {};
            if (invalidSaved) host.saved = {{"keyblade", -2}, {"character", 9}};
            host.failAt = failure;
            assert(!settings.initialize(ctx, &config, &ui, error));
            assert(!error.empty() && host.vars.empty() && host.subscriptions.empty());
            assert(!settings.takePending(pending));
            settings.shutdown();
        }
    }
    host = {};
    assert(!settings.initialize(ctx, nullptr, &ui, error));
    assert(!settings.initialize(ctx, &config, nullptr, error));
    assert(settings.initialize(ctx, &config, &ui, error));
    settings.shutdown();
    assert(host.vars.empty() && host.subscriptions.empty());
    std::puts("PASS: persisted defaults/reload, independent deferred choices, stable dropdown IDs, invalid-ID recovery, panel rebuild/status, teardown, and all 16 partial-initialization failures.");
}
