#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <ShlObj.h>
#include <Windows.h>
#endif

#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"

#include "imguiCustom.h"

#include "GUI.h"
#include "InventoryChanger/InventoryChanger.h"
#include "Helpers.h"
#include "Interfaces.h"
#include "SDK/InputSystem.h"
#include "Hooks.h"
#include "Hacks/Misc.h"
#include "Hacks/Visuals.h"
#include "Config.h"
#include "SDK/LocalPlayer.h"
#include "SDK/Entity.h"
#include "SDK/Engine.h"
#include "GameData.h"
#include "Memory.h"
#include "SDK/GlobalVars.h"

constexpr auto windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

static ImFont* addFontFromVFONT(const std::string& path, float size, const ImWchar* glyphRanges, bool merge) noexcept {
    auto file = Helpers::loadBinaryFile(path);
    if (!Helpers::decodeVFONT(file))
        return nullptr;

    ImFontConfig cfg;
    cfg.FontData = file.data();
    cfg.FontDataSize = file.size();
    cfg.FontDataOwnedByAtlas = false;
    cfg.MergeMode = merge;
    cfg.GlyphRanges = glyphRanges;
    cfg.SizePixels = size;

    return ImGui::GetIO().Fonts->AddFont(&cfg);
}

GUI::GUI() noexcept {
    ImGui::StyleColorsMain();
    ImGuiStyle& style = ImGui::GetStyle();

    style.ScrollbarSize = 9.0f;

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImFontConfig cfg;
    cfg.SizePixels = 15.0f;

#ifdef _WIN32
    if (PWSTR pathToFonts; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &pathToFonts))) {
        const std::filesystem::path path{ pathToFonts };
        CoTaskMemFree(pathToFonts);

        fonts.normal15px = io.Fonts->AddFontFromFileTTF((path / "tahoma.ttf").string().c_str(), 15.0f, &cfg, Helpers::getFontGlyphRanges());
        if (!fonts.normal15px)
            io.Fonts->AddFontDefault(&cfg);

        cfg.MergeMode = true;
        static constexpr ImWchar symbol[]{
            0x2605, 0x2605, // ★
            0
        };
        io.Fonts->AddFontFromFileTTF((path / "seguisym.ttf").string().c_str(), 15.0f, &cfg, symbol);
        cfg.MergeMode = false;
    }
#else
    fonts.normal15px = addFontFromVFONT("csgo/panorama/fonts/notosans-regular.vfont", 15.0f, Helpers::getFontGlyphRanges(), false);
#endif
    if (!fonts.normal15px)
        io.Fonts->AddFontDefault(&cfg);
    addFontFromVFONT("csgo/panorama/fonts/notosanskr-regular.vfont", 15.0f, io.Fonts->GetGlyphRangesKorean(), true);
    addFontFromVFONT("csgo/panorama/fonts/notosanssc-regular.vfont", 17.0f, io.Fonts->GetGlyphRangesChineseFull(), true);
}

void GUI::render() noexcept {
    renderMenuBar();

    renderAimbotWindow();
    renderTriggerbotWindow();
    renderBacktrackWindow();
    // renderAntiAimWindow(); TODO
    renderGlowWindow();
    renderChamsWindow();
    renderESPWindow();
    renderVisualsWindow();
    InventoryChanger::drawGUI(false);
    renderMiscWindow();
    renderPlayerListWindow();
    renderStyleWindow();
    renderConfigWindow();
}

void GUI::updateColors() const noexcept {
    switch (config->styleConfig.menuColours) {
        case 0: ImGui::StyleColorsMain(); break;
        case 1: ImGui::StyleColorsDark(); break;
        case 2: ImGui::StyleColorsLight(); break;
    }
}

void GUI::handleToggle() noexcept {
    if (Misc::isMenuKeyPressed()) {
        open = !open;
        if (!open)
            interfaces->inputSystem->resetInputState();
#ifndef _WIN32
        ImGui::GetIO().MouseDrawCursor = gui->open;
#endif
    }
}

static void menuBarItem(const char* name, bool& enabled) noexcept {
    if (ImGui::MenuItem(name)) {
        enabled = true;
        ImGui::SetWindowFocus(name);
        ImGui::SetWindowPos(name, { 100.0f, 100.0f });
    }
}

void GUI::renderMenuBar() noexcept {
    if (ImGui::BeginMainMenuBar()) {
        ImGui::PushStyleColor(ImGuiCol_Text, Helpers::calculateColor(config->styleConfig.menuColour.asColor3()));
        ImGui::TextWrapped("uwu");
        ImGui::PopStyleColor();

        ImGui::Separator();

        menuBarItem("Aimbot", window.aimbot);
        menuBarItem("Triggerbot", window.triggerbot);
        menuBarItem("Backtrack", window.backtrack);
        // menuBarItem("Anti Aim", window.antiAim); TODO
        menuBarItem("Glow", window.glow);
        menuBarItem("Chams", window.chams);
        menuBarItem("ESP", window.esp);
        menuBarItem("Visuals", window.visuals);
        InventoryChanger::menuBarItem();
        menuBarItem("Misc", window.misc);
        menuBarItem("Player List", window.playerList);
        menuBarItem("Style", window.style);
        menuBarItem("Config", window.config);
        ImGui::EndMainMenuBar();
    }
}

// PLAYER LIST

void GUI::renderPlayerListWindow(bool contentOnly) noexcept {
    if (!contentOnly) {
        if (!window.playerList)
            return;
        ImGui::SetNextWindowSize({ 600.0f, 0.0f });
        ImGui::Begin("Player List", &window.playerList, windowFlags);
    }

    // Check to see if the player is in a match.
    if (!interfaces->engine->isConnected() || GameData::players().size() <= 2) {
        ImGui::Text("Failed to get the players in the match!");
        ImGui::End();
        return;
    }

    if (ImGui::BeginTable("Player Table", 4)) {
        // Set up table columns
        ImGui::TableSetupColumn("Player", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Health", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
        // Pop header
        ImGui::TableHeadersRow();

        // For every player in the game, add a table item
        for (int row = 0; row < GameData::players().size(); row++) {
            // If we can find the actual players' information
            if (const auto it = std::ranges::find(GameData::players(), GameData::players()[row].handle, &PlayerData::handle); it != GameData::players().cend()) {
                ImGui::TableNextRow();
                for (int column = 0; column < 4; column++) {
                    ImGui::TableSetColumnIndex(column);

                    // Specify content for each header
                    switch (column) {
                        case 0:
                            if (const auto texture = it->getAvatarTexture()) {
                                const auto textSize = ImGui::CalcTextSize(it->name.c_str());
                                ImGui::Image(texture, ImVec2(textSize.y, textSize.y), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.3f));
                                ImGui::SameLine();
                                ImGui::Text("%s", it->name.c_str());
                            }
                            break;
                        case 1:
                            ImGui::Text("%d", it->health);
                            break;
                        case 2:
                            ImGui::Text("%d", it->alive);
                            break;
                        case 3:
                            ImGui::PushID(row);
                            if (ImGui::Button("View")) {
                                if (ImGui::BeginPopupModal("Player")) {
                                    ImGui::Text("Player Health\t%d", it->health);
                                    ImGui::Text("Is Alive?\t%d", it->alive);
                                    ImGui::Text("Is Audible?\t%d", it->audible);
                                    ImGui::Text("Player Distance\t%d", it->distanceToLocal);
                                    ImGui::EndPopup();
                                }
                            }
                            ImGui::PopID();
                            break;
                    }
                }
            }
        }
        ImGui::EndTable();
    }

    if (!contentOnly)
        ImGui::End();
}

// AIMBOT

void GUI::renderAimbotWindow(bool contentOnly) noexcept {
    if (!contentOnly) {
        if (!window.aimbot)
            return;
        ImGui::SetNextWindowSize({ 600.0f, 0.0f });
        ImGui::Begin("Aimbot", &window.aimbot, windowFlags);
    }
    ImGui::Checkbox("On Key", &config->aimbotOnKey);
    ImGui::SameLine();
    ImGui::PushID("Aimbot Key");
    ImGui::hotkey("", config->aimbotKey);
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::PushID(2);
    ImGui::PushItemWidth(70.0f);
    ImGui::Combo("", &config->aimbotKeyMode, "Hold\0Toggle\0");
    ImGui::PopItemWidth();
    ImGui::PopID();
    ImGui::Separator();
    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);
    ImGui::Combo("", &currentCategory, "All\0Pistols\0Heavy\0SMG\0Rifles\0");
    ImGui::PopID();
    ImGui::SameLine();
    static int currentWeapon{ 0 };
    ImGui::PushID(1);

    switch (currentCategory) {
        case 0:
            currentWeapon = 0;
            ImGui::NewLine();
            break;
        case 1:
        {
            static int currentPistol{ 0 };
            static constexpr const char* pistols[]{ "All", "Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-Seven", "CZ-75", "Desert Eagle", "Revolver" };

            ImGui::Combo("", &currentPistol, [](void*, int idx, const char** out_text) {
                if (config->aimbotConfig[idx ? idx : 35].enabled) {
                    static std::string name;
                    name = pistols[idx];
                    *out_text = name.append(" *").c_str();
                }
                else {
                    *out_text = pistols[idx];
                }
                return true;
                         }, nullptr, IM_ARRAYSIZE(pistols));

            currentWeapon = currentPistol ? currentPistol : 35;
            break;
        }
        case 2:
        {
            static int currentHeavy{ 0 };
            static constexpr const char* heavies[]{ "All", "Nova", "XM1014", "Sawed-off", "MAG-7", "M249", "Negev" };

            ImGui::Combo("", &currentHeavy, [](void*, int idx, const char** out_text) {
                if (config->aimbotConfig[idx ? idx + 10 : 36].enabled) {
                    static std::string name;
                    name = heavies[idx];
                    *out_text = name.append(" *").c_str();
                }
                else {
                    *out_text = heavies[idx];
                }
                return true;
                         }, nullptr, IM_ARRAYSIZE(heavies));

            currentWeapon = currentHeavy ? currentHeavy + 10 : 36;
            break;
        }
        case 3:
        {
            static int currentSmg{ 0 };
            static constexpr const char* smgs[]{ "All", "Mac-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon" };

            ImGui::Combo("", &currentSmg, [](void*, int idx, const char** out_text) {
                if (config->aimbotConfig[idx ? idx + 16 : 37].enabled) {
                    static std::string name;
                    name = smgs[idx];
                    *out_text = name.append(" *").c_str();
                }
                else {
                    *out_text = smgs[idx];
                }
                return true;
                         }, nullptr, IM_ARRAYSIZE(smgs));

            currentWeapon = currentSmg ? currentSmg + 16 : 37;
            break;
        }
        case 4:
        {
            static int currentRifle{ 0 };
            static constexpr const char* rifles[]{ "All", "Galil AR", "Famas", "AK-47", "M4A4", "M4A1-S", "SSG-08", "SG-553", "AUG", "AWP", "G3SG1", "SCAR-20" };

            ImGui::Combo("", &currentRifle, [](void*, int idx, const char** out_text) {
                if (config->aimbotConfig[idx ? idx + 23 : 38].enabled) {
                    static std::string name;
                    name = rifles[idx];
                    *out_text = name.append(" *").c_str();
                }
                else {
                    *out_text = rifles[idx];
                }
                return true;
                         }, nullptr, IM_ARRAYSIZE(rifles));

            currentWeapon = currentRifle ? currentRifle + 23 : 38;
            break;
        }
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::Checkbox("Enabled", &config->aimbotConfig[currentWeapon].enabled);
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 220.0f);
    ImGui::Checkbox("Aimlock", &config->aimbotConfig[currentWeapon].aimlock);
    ImGui::Checkbox("Silent", &config->aimbotConfig[currentWeapon].silent);
    ImGui::Checkbox("Friendly Fire", &config->aimbotConfig[currentWeapon].friendlyFire);
    ImGui::Checkbox("Visible Only", &config->aimbotConfig[currentWeapon].visibleOnly);
    ImGui::Checkbox("Scoped Only", &config->aimbotConfig[currentWeapon].scopedOnly);
    ImGui::Checkbox("Ignore Flash", &config->aimbotConfig[currentWeapon].ignoreFlash);
    ImGui::Checkbox("Ignore Smoke", &config->aimbotConfig[currentWeapon].ignoreSmoke);
    ImGui::Checkbox("Auto Shot", &config->aimbotConfig[currentWeapon].autoShot);
    ImGui::Checkbox("Auto Scope", &config->aimbotConfig[currentWeapon].autoScope);
    ImGui::Combo("Bone", &config->aimbotConfig[currentWeapon].bone, "Nearest\0Best Damage\0Head\0Neck\0Sternum\0Chest\0Stomach\0Pelvis\0");
    ImGui::NextColumn();
    ImGui::PushItemWidth(240.0f);
    ImGui::SliderFloat("FOV", &config->aimbotConfig[currentWeapon].fov, 0.0f, 255.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("Smooth", &config->aimbotConfig[currentWeapon].smooth, 1.0f, 100.0f, "%.2f");
    ImGui::SliderFloat("Max Aim Inaccuracy", &config->aimbotConfig[currentWeapon].maxAimInaccuracy, 0.0f, 1.0f, "%.5f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("Max Shot Inaccuracy", &config->aimbotConfig[currentWeapon].maxShotInaccuracy, 0.0f, 1.0f, "%.5f", ImGuiSliderFlags_Logarithmic);
    ImGui::InputInt("Minimum damage", &config->aimbotConfig[currentWeapon].minDamage);
    config->aimbotConfig[currentWeapon].minDamage = std::clamp(config->aimbotConfig[currentWeapon].minDamage, 0, 250);
    ImGui::Checkbox("Killshot", &config->aimbotConfig[currentWeapon].killshot);
    ImGui::Checkbox("Between Shots", &config->aimbotConfig[currentWeapon].betweenShots);
    ImGui::Columns(1);
    if (!contentOnly)
        ImGui::End();
}

// TRIGGERBOT

void GUI::renderTriggerbotWindow(bool contentOnly) noexcept {
    if (!contentOnly) {
        if (!window.triggerbot)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Triggerbot", &window.triggerbot, windowFlags);
    }
    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);
    ImGui::Combo("", &currentCategory, "All\0Pistols\0Heavy\0SMG\0Rifles\0Zeus x27\0");
    ImGui::PopID();
    ImGui::SameLine();
    static int currentWeapon{ 0 };
    ImGui::PushID(1);
    switch (currentCategory) {
        case 0:
            currentWeapon = 0;
            ImGui::NewLine();
            break;
        case 5:
            currentWeapon = 39;
            ImGui::NewLine();
            break;

        case 1:
        {
            static int currentPistol{ 0 };
            static constexpr const char* pistols[]{ "All", "Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-Seven", "CZ-75", "Desert Eagle", "Revolver" };

            ImGui::Combo("", &currentPistol, [](void*, int idx, const char** out_text) {
                if (config->triggerbotConfig[idx ? idx : 35].enabled) {
                    static std::string name;
                    name = pistols[idx];
                    *out_text = name.append(" *").c_str();
                }
                else {
                    *out_text = pistols[idx];
                }
                return true;
                         }, nullptr, IM_ARRAYSIZE(pistols));

            currentWeapon = currentPistol ? currentPistol : 35;
            break;
        }
        case 2:
        {
            static int currentHeavy{ 0 };
            static constexpr const char* heavies[]{ "All", "Nova", "XM1014", "Sawed-off", "MAG-7", "M249", "Negev" };

            ImGui::Combo("", &currentHeavy, [](void*, int idx, const char** out_text) {
                if (config->triggerbotConfig[idx ? idx + 10 : 36].enabled) {
                    static std::string name;
                    name = heavies[idx];
                    *out_text = name.append(" *").c_str();
                }
                else {
                    *out_text = heavies[idx];
                }
                return true;
                         }, nullptr, IM_ARRAYSIZE(heavies));

            currentWeapon = currentHeavy ? currentHeavy + 10 : 36;
            break;
        }
        case 3:
        {
            static int currentSmg{ 0 };
            static constexpr const char* smgs[]{ "All", "Mac-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon" };

            ImGui::Combo("", &currentSmg, [](void*, int idx, const char** out_text) {
                if (config->triggerbotConfig[idx ? idx + 16 : 37].enabled) {
                    static std::string name;
                    name = smgs[idx];
                    *out_text = name.append(" *").c_str();
                }
                else {
                    *out_text = smgs[idx];
                }
                return true;
                         }, nullptr, IM_ARRAYSIZE(smgs));

            currentWeapon = currentSmg ? currentSmg + 16 : 37;
            break;
        }
        case 4:
        {
            static int currentRifle{ 0 };
            static constexpr const char* rifles[]{ "All", "Galil AR", "Famas", "AK-47", "M4A4", "M4A1-S", "SSG-08", "SG-553", "AUG", "AWP", "G3SG1", "SCAR-20" };

            ImGui::Combo("", &currentRifle, [](void*, int idx, const char** out_text) {
                if (config->triggerbotConfig[idx ? idx + 23 : 38].enabled) {
                    static std::string name;
                    name = rifles[idx];
                    *out_text = name.append(" *").c_str();
                }
                else {
                    *out_text = rifles[idx];
                }
                return true;
                         }, nullptr, IM_ARRAYSIZE(rifles));

            currentWeapon = currentRifle ? currentRifle + 23 : 38;
            break;
        }
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::Checkbox("Enabled", &config->triggerbotConfig[currentWeapon].enabled);
    ImGui::Separator();
    ImGui::hotkey("Hold Key", config->triggerbotHoldKey);
    ImGui::Checkbox("Friendly Fire", &config->triggerbotConfig[currentWeapon].friendlyFire);
    ImGui::Checkbox("Scoped Only", &config->triggerbotConfig[currentWeapon].scopedOnly);
    ImGui::Checkbox("Ignore Flash", &config->triggerbotConfig[currentWeapon].ignoreFlash);
    ImGui::Checkbox("Ignore Smoke", &config->triggerbotConfig[currentWeapon].ignoreSmoke);
    ImGui::SetNextItemWidth(85.0f);
    ImGui::Combo("Hitgroup", &config->triggerbotConfig[currentWeapon].hitgroup, "All\0Head\0Chest\0Stomach\0Left Arm\0Right Arm\0Left Leg\0Right Leg\0");
    ImGui::PushItemWidth(220.0f);
    ImGui::SliderInt("Shot Delay", &config->triggerbotConfig[currentWeapon].shotDelay, 0, 250, "%d ms");
    ImGui::InputInt("Minimum Damage", &config->triggerbotConfig[currentWeapon].minDamage);
    config->triggerbotConfig[currentWeapon].minDamage = std::clamp(config->triggerbotConfig[currentWeapon].minDamage, 0, 250);
    ImGui::Checkbox("Killshot", &config->triggerbotConfig[currentWeapon].killshot);
    ImGui::SliderFloat("Burst Time", &config->triggerbotConfig[currentWeapon].burstTime, 0.0f, 0.5f, "%.3f s");

    if (!contentOnly)
        ImGui::End();
}

// BACKTRACK

void GUI::renderBacktrackWindow(bool contentOnly) noexcept {
    if (!contentOnly) {
        if (!window.backtrack)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Backtrack", &window.backtrack, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    }
    ImGui::Checkbox("Enabled", &config->backtrackConfig.enabled);
    ImGui::Checkbox("Ignore Smoke", &config->backtrackConfig.ignoreSmoke);
    ImGui::Checkbox("Recoil Based FOV", &config->backtrackConfig.recoilBasedFov);
    ImGui::PushItemWidth(220.0f);
    ImGui::SliderInt("Limit", &config->backtrackConfig.timeLimit, 1, 200, "%d ms");
    ImGui::PopItemWidth();
    if (!contentOnly)
        ImGui::End();
}

// ANTI AIM TODO

//void GUI::renderAntiAimWindow(bool contentOnly) noexcept {
//    if (!contentOnly) {
//        if (!window.antiAim)
//            return;
//        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
//        ImGui::Begin("Anti Aim", &window.antiAim, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
//    }
//    ImGui::Checkbox("Enabled", &config->antiAimConfig.enabled);
//    if (!contentOnly)
//        ImGui::End();
//}

// GLOW

void GUI::renderGlowWindow(bool contentOnly) noexcept {
    if (!contentOnly) {
        if (!window.glow)
            return;
        ImGui::SetNextWindowSize({ 450.0f, 0.0f });
        ImGui::Begin("Glow", &window.glow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    }

    ImGui::hotkey("Toggle Key", config->glowToggleKey, 80.0f);
    ImGui::hotkey("Hold Key", config->glowHoldKey, 80.0f);
    ImGui::Separator();

    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);
    constexpr std::array categories{ "Allies", "Enemies", "Planting", "Defusing", "Local Player", "Weapons", "C4", "Planted C4", "Chickens", "Defuse Kits", "Projectiles", "Hostages", "Ragdolls" };
    ImGui::Combo("", &currentCategory, categories.data(), categories.size());
    ImGui::PopID();
    Config::GlowItem* currentItem{};
    if (currentCategory <= 3) {
        ImGui::SameLine();
        static int currentType{ 0 };
        ImGui::PushID(1);
        ImGui::Combo("", &currentType, "All\0Visible\0Occluded\0");
        ImGui::PopID();
        auto& cfg = config->playerGlowConfig[categories[currentCategory]];
        switch (currentType) {
            case 0: currentItem = &cfg.all; break;
            case 1: currentItem = &cfg.visible; break;
            case 2: currentItem = &cfg.occluded; break;
        }
    }
    else {
        currentItem = &config->glowConfig[categories[currentCategory]];
    }

    ImGui::SameLine();
    ImGui::Checkbox("Enabled", &currentItem->enabled);
    ImGui::Separator();
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 150.0f);
    ImGui::Checkbox("Health Based", &currentItem->healthBased);

    ImGuiCustom::colorPicker("Colour", *currentItem);

    ImGui::NextColumn();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::Combo("Style", &currentItem->style, "Default\0Rim 3D\0Edge\0Edge Pulse\0");

    ImGui::Columns(1);
    if (!contentOnly)
        ImGui::End();
}

// CHAMS

void GUI::renderChamsWindow(bool contentOnly) noexcept {
    if (!contentOnly) {
        if (!window.chams)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Chams", &window.chams, windowFlags);
    }

    ImGui::hotkey("Toggle Key", config->chamsToggleKey, 80.0f);
    ImGui::hotkey("Hold Key", config->chamsHoldKey, 80.0f);
    ImGui::Separator();

    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);

    static int material = 1;

    if (ImGui::Combo("", &currentCategory, "Allies\0Enemies\0Planting\0Defusing\0Local Player\0Weapons\0Hands\0Backtrack\0Sleeves\0"))
        material = 1;

    ImGui::PopID();

    ImGui::SameLine();

    if (material <= 1)
        ImGuiCustom::arrowButtonDisabled("##left", ImGuiDir_Left);
    else if (ImGui::ArrowButton("##left", ImGuiDir_Left))
        --material;

    ImGui::SameLine();
    ImGui::Text("%d", material);

    constexpr std::array categories{ "Allies", "Enemies", "Planting", "Defusing", "Local Player", "Weapons", "Hands", "Backtrack", "Sleeves" };

    ImGui::SameLine();

    if (material >= int(config->chamsConfig[categories[currentCategory]].materials.size()))
        ImGuiCustom::arrowButtonDisabled("##right", ImGuiDir_Right);
    else if (ImGui::ArrowButton("##right", ImGuiDir_Right))
        ++material;

    ImGui::SameLine();

    auto& chams{ config->chamsConfig[categories[currentCategory]].materials[material - 1] };

    ImGui::Checkbox("Enabled", &chams.enabled);
    ImGui::Separator();
    ImGui::Checkbox("Health Based", &chams.healthBased);
    ImGui::Checkbox("Blinking", &chams.blinking);
    ImGui::Combo("Material", &chams.material, "Normal\0Flat\0Animated\0Platinum\0Glass\0Chrome\0Crystal\0Silver\0Gold\0Plastic\0Glow\0Pearlescent\0Metallic\0");
    ImGui::Checkbox("Wireframe", &chams.wireframe);
    ImGui::Checkbox("Cover", &chams.cover);
    ImGui::Checkbox("Ignore-Z", &chams.ignorez);
    ImGuiCustom::colorPicker("Colour", chams);

    if (!contentOnly) {
        ImGui::End();
    }
}

// ESP

void GUI::renderESPWindow(bool contentOnly) noexcept {
    if (!contentOnly) {
        if (!window.esp)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("ESP", &window.esp, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    }

    ImGui::hotkey("Toggle Key", config->espConfig.toggleKey, 80.0f);
    ImGui::hotkey("Hold Key", config->espConfig.holdKey, 80.0f);
    ImGui::Separator();

    static std::size_t currentCategory;
    static auto currentItem = "All";

    constexpr auto getConfigShared = [](std::size_t category, const char* item) noexcept -> Shared& {
        switch (category) {
            case 0: default: return config->espConfig.enemies[item];
            case 1: return config->espConfig.allies[item];
            case 2: return config->espConfig.weapons[item];
            case 3: return config->espConfig.projectiles[item];
            case 4: return config->espConfig.lootCrates[item];
            case 5: return config->espConfig.otherEntities[item];
        }
    };

    constexpr auto getConfigPlayer = [](std::size_t category, const char* item) noexcept -> Player& {
        switch (category) {
            case 0: default: return config->espConfig.enemies[item];
            case 1: return config->espConfig.allies[item];
        }
    };

    if (ImGui::BeginListBox("##list", { 170.0f, 300.0f })) {
        constexpr std::array categories{ "Enemies", "Allies", "Weapons", "Projectiles", "Loot Crates", "Other Entities" };

        for (std::size_t i = 0; i < categories.size(); ++i) {
            if (ImGui::Selectable(categories[i], currentCategory == i && std::string_view{ currentItem } == "All")) {
                currentCategory = i;
                currentItem = "All";
            }

            if (ImGui::BeginDragDropSource()) {
                switch (i) {
                    case 0: case 1: ImGui::SetDragDropPayload("Player", &getConfigPlayer(i, "All"), sizeof(Player), ImGuiCond_Once); break;
                    case 2: ImGui::SetDragDropPayload("Weapon", &config->espConfig.weapons["All"], sizeof(Weapon), ImGuiCond_Once); break;
                    case 3: ImGui::SetDragDropPayload("Projectile", &config->espConfig.projectiles["All"], sizeof(Projectile), ImGuiCond_Once); break;
                    default: ImGui::SetDragDropPayload("Entity", &getConfigShared(i, "All"), sizeof(Shared), ImGuiCond_Once); break;
                }
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Player")) {
                    const auto& data = *(Player*)payload->Data;

                    switch (i) {
                        case 0: case 1: getConfigPlayer(i, "All") = data; break;
                        case 2: config->espConfig.weapons["All"] = data; break;
                        case 3: config->espConfig.projectiles["All"] = data; break;
                        default: getConfigShared(i, "All") = data; break;
                    }
                }

                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Weapon")) {
                    const auto& data = *(Weapon*)payload->Data;

                    switch (i) {
                        case 0: case 1: getConfigPlayer(i, "All") = data; break;
                        case 2: config->espConfig.weapons["All"] = data; break;
                        case 3: config->espConfig.projectiles["All"] = data; break;
                        default: getConfigShared(i, "All") = data; break;
                    }
                }

                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Projectile")) {
                    const auto& data = *(Projectile*)payload->Data;

                    switch (i) {
                        case 0: case 1: getConfigPlayer(i, "All") = data; break;
                        case 2: config->espConfig.weapons["All"] = data; break;
                        case 3: config->espConfig.projectiles["All"] = data; break;
                        default: getConfigShared(i, "All") = data; break;
                    }
                }

                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity")) {
                    const auto& data = *(Shared*)payload->Data;

                    switch (i) {
                        case 0: case 1: getConfigPlayer(i, "All") = data; break;
                        case 2: config->espConfig.weapons["All"] = data; break;
                        case 3: config->espConfig.projectiles["All"] = data; break;
                        default: getConfigShared(i, "All") = data; break;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PushID(i);
            ImGui::Indent();

            const auto items = [](std::size_t category) noexcept -> std::vector<const char*> {
                switch (category) {
                    case 0:
                    case 1: return { "Visible", "Occluded" };
                    case 2: return { "Pistols", "SMGs", "Rifles", "Sniper Rifles", "Shotguns", "Machineguns", "Grenades", "Melee", "Other" };
                    case 3: return { "Flashbang", "HE Grenade", "Breach Charge", "Bump Mine", "Decoy Grenade", "Molotov", "TA Grenade", "Smoke Grenade", "Snowball" };
                    case 4: return { "Pistol Case", "Light Case", "Heavy Case", "Explosive Case", "Tools Case", "Cash Dufflebag" };
                    case 5: return { "Defuse Kit", "Chicken", "Planted C4", "Hostage", "Sentry", "Cash", "Ammo Box", "Radar Jammer", "Snowball Pile", "Collectable Coin" };
                    default: return {};
                }
            }(i);

            const auto categoryEnabled = getConfigShared(i, "All").enabled;

            for (std::size_t j = 0; j < items.size(); ++j) {
                static bool selectedSubItem;
                if (!categoryEnabled || getConfigShared(i, items[j]).enabled) {
                    if (ImGui::Selectable(items[j], currentCategory == i && !selectedSubItem && std::string_view{ currentItem } == items[j])) {
                        currentCategory = i;
                        currentItem = items[j];
                        selectedSubItem = false;
                    }

                    if (ImGui::BeginDragDropSource()) {
                        switch (i) {
                            case 0: case 1: ImGui::SetDragDropPayload("Player", &getConfigPlayer(i, items[j]), sizeof(Player), ImGuiCond_Once); break;
                            case 2: ImGui::SetDragDropPayload("Weapon", &config->espConfig.weapons[items[j]], sizeof(Weapon), ImGuiCond_Once); break;
                            case 3: ImGui::SetDragDropPayload("Projectile", &config->espConfig.projectiles[items[j]], sizeof(Projectile), ImGuiCond_Once); break;
                            default: ImGui::SetDragDropPayload("Entity", &getConfigShared(i, items[j]), sizeof(Shared), ImGuiCond_Once); break;
                        }
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Player")) {
                            const auto& data = *(Player*)payload->Data;

                            switch (i) {
                                case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                                case 2: config->espConfig.weapons[items[j]] = data; break;
                                case 3: config->espConfig.projectiles[items[j]] = data; break;
                                default: getConfigShared(i, items[j]) = data; break;
                            }
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Weapon")) {
                            const auto& data = *(Weapon*)payload->Data;

                            switch (i) {
                                case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                                case 2: config->espConfig.weapons[items[j]] = data; break;
                                case 3: config->espConfig.projectiles[items[j]] = data; break;
                                default: getConfigShared(i, items[j]) = data; break;
                            }
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Projectile")) {
                            const auto& data = *(Projectile*)payload->Data;

                            switch (i) {
                                case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                                case 2: config->espConfig.weapons[items[j]] = data; break;
                                case 3: config->espConfig.projectiles[items[j]] = data; break;
                                default: getConfigShared(i, items[j]) = data; break;
                            }
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity")) {
                            const auto& data = *(Shared*)payload->Data;

                            switch (i) {
                                case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                                case 2: config->espConfig.weapons[items[j]] = data; break;
                                case 3: config->espConfig.projectiles[items[j]] = data; break;
                                default: getConfigShared(i, items[j]) = data; break;
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                }

                if (i != 2)
                    continue;

                ImGui::Indent();

                const auto subItems = [](std::size_t item) noexcept -> std::vector<const char*> {
                    switch (item) {
                        case 0: return { "Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-SeveN", "CZ75-Auto", "Desert Eagle", "R8 Revolver" };
                        case 1: return { "MAC-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon" };
                        case 2: return { "Galil AR", "FAMAS", "AK-47", "M4A4", "M4A1-S", "SG 553", "AUG" };
                        case 3: return { "SSG 08", "AWP", "G3SG1", "SCAR-20" };
                        case 4: return { "Nova", "XM1014", "Sawed-Off", "MAG-7" };
                        case 5: return { "M249", "Negev" };
                        case 6: return { "Flashbang", "HE Grenade", "Smoke Grenade", "Molotov", "Decoy Grenade", "Incendiary", "TA Grenade", "Fire Bomb", "Diversion", "Frag Grenade", "Snowball" };
                        case 7: return { "Axe", "Hammer", "Wrench" };
                        case 8: return { "C4", "Healthshot", "Bump Mine", "Zone Repulsor", "Shield" };
                        default: return {};
                    }
                }(j);

                const auto itemEnabled = getConfigShared(i, items[j]).enabled;

                for (const auto subItem : subItems) {
                    auto& subItemConfig = config->espConfig.weapons[subItem];
                    if ((categoryEnabled || itemEnabled) && !subItemConfig.enabled)
                        continue;

                    if (ImGui::Selectable(subItem, currentCategory == i && selectedSubItem && std::string_view{ currentItem } == subItem)) {
                        currentCategory = i;
                        currentItem = subItem;
                        selectedSubItem = true;
                    }

                    if (ImGui::BeginDragDropSource()) {
                        ImGui::SetDragDropPayload("Weapon", &subItemConfig, sizeof(Weapon), ImGuiCond_Once);
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Player")) {
                            const auto& data = *(Player*)payload->Data;
                            subItemConfig = data;
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Weapon")) {
                            const auto& data = *(Weapon*)payload->Data;
                            subItemConfig = data;
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Projectile")) {
                            const auto& data = *(Projectile*)payload->Data;
                            subItemConfig = data;
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity")) {
                            const auto& data = *(Shared*)payload->Data;
                            subItemConfig = data;
                        }
                        ImGui::EndDragDropTarget();
                    }
                }

                ImGui::Unindent();
            }
            ImGui::Unindent();
            ImGui::PopID();
        }
        ImGui::EndListBox();
    }

    ImGui::SameLine();

    if (ImGui::BeginChild("##child", { 400.0f, 0.0f })) {
        auto& sharedConfig = getConfigShared(currentCategory, currentItem);

        ImGui::Checkbox("Enabled", &sharedConfig.enabled);
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 260.0f);
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::BeginCombo("Font", config->getSystemFonts()[sharedConfig.font.index].c_str())) {
            for (size_t i = 0; i < config->getSystemFonts().size(); i++) {
                bool isSelected = config->getSystemFonts()[i] == sharedConfig.font.name;
                if (ImGui::Selectable(config->getSystemFonts()[i].c_str(), isSelected, 0, { 250.0f, 0.0f })) {
                    sharedConfig.font.index = i;
                    sharedConfig.font.name = config->getSystemFonts()[i];
                    config->scheduleFontLoad(sharedConfig.font.name);
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        constexpr auto spacing = 250.0f;
        ImGuiCustom::colorPicker("Snapline", sharedConfig.snapline);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90.0f);
        ImGui::Combo("##1", &sharedConfig.snapline.type, "Bottom\0Top\0Crosshair\0");
        ImGui::SameLine(spacing);
        ImGuiCustom::colorPicker("Box", sharedConfig.box);
        ImGui::SameLine();

        ImGui::PushID("Box");

        if (ImGui::Button("..."))
            ImGui::OpenPopup("");

        if (ImGui::BeginPopup("")) {
            ImGui::SetNextItemWidth(95.0f);
            ImGui::Combo("Type", &sharedConfig.box.type, "2D\0" "2D Corners\0" "3D\0" "3D Corners\0");
            ImGui::SetNextItemWidth(275.0f);
            ImGui::SliderFloat3("Scale", sharedConfig.box.scale.data(), 0.0f, 0.50f, "%.2f");
            ImGuiCustom::colorPicker("Fill", sharedConfig.box.fill);
            ImGui::EndPopup();
        }

        ImGui::PopID();

        ImGuiCustom::colorPicker("Name", sharedConfig.name);
        ImGui::SameLine(spacing);

        if (currentCategory < 2) {
            auto& playerConfig = getConfigPlayer(currentCategory, currentItem);

            ImGuiCustom::colorPicker("Weapon", playerConfig.weapon);
            ImGuiCustom::colorPicker("Flash Duration", playerConfig.flashDuration);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Skeleton", playerConfig.skeleton);
            ImGui::Checkbox("Audible Only", &playerConfig.audibleOnly);
            ImGui::SameLine(spacing);
            ImGui::Checkbox("Spotted Only", &playerConfig.spottedOnly);

            ImGuiCustom::colorPicker("Head Box", playerConfig.headBox);
            ImGui::SameLine();

            ImGui::PushID("Head Box");

            if (ImGui::Button("..."))
                ImGui::OpenPopup("");

            if (ImGui::BeginPopup("")) {
                ImGui::SetNextItemWidth(95.0f);
                ImGui::Combo("Type", &playerConfig.headBox.type, "2D\0" "2D Corners\0" "3D\0" "3D Corners\0");
                ImGui::SetNextItemWidth(275.0f);
                ImGui::SliderFloat3("Scale", playerConfig.headBox.scale.data(), 0.0f, 0.50f, "%.2f");
                ImGuiCustom::colorPicker("Fill", playerConfig.headBox.fill);
                ImGui::EndPopup();
            }

            ImGui::PopID();

            ImGui::SameLine(spacing);
            ImGui::Checkbox("Health Bar", &playerConfig.healthBar.enabled);
            ImGui::SameLine();

            ImGui::PushID("Health Bar");

            if (ImGui::Button("..."))
                ImGui::OpenPopup("");

            if (ImGui::BeginPopup("")) {
                ImGui::SetNextItemWidth(95.0f);
                ImGui::Combo("Type", &playerConfig.healthBar.type, "Gradient\0Solid\0Health Based\0");
                if (playerConfig.healthBar.type == HealthBar::Solid) {
                    ImGui::SameLine();
                    ImGuiCustom::colorPicker("", playerConfig.healthBar.asColor4());
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }
        else if (currentCategory == 2) {
            auto& weaponConfig = config->espConfig.weapons[currentItem];
            ImGuiCustom::colorPicker("Ammo", weaponConfig.ammo);
        }
        else if (currentCategory == 3) {
            auto& trails = config->espConfig.projectiles[currentItem].trails;

            ImGui::Checkbox("Trails", &trails.enabled);
            ImGui::SameLine(spacing + 77.0f);
            ImGui::PushID("Trails");

            if (ImGui::Button("..."))
                ImGui::OpenPopup("");

            if (ImGui::BeginPopup("")) {
                constexpr auto trailPicker = [](const char* name, Trail& trail) noexcept {
                    ImGui::PushID(name);
                    ImGuiCustom::colorPicker(name, trail);
                    ImGui::SameLine(150.0f);
                    ImGui::SetNextItemWidth(95.0f);
                    ImGui::Combo("", &trail.type, "Line\0Circles\0Filled Circles\0");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(95.0f);
                    ImGui::InputFloat("Time", &trail.time, 0.1f, 0.5f, "%.1fs");
                    trail.time = std::clamp(trail.time, 1.0f, 60.0f);
                    ImGui::PopID();
                };

                trailPicker("Local Player", trails.localPlayer);
                trailPicker("Allies", trails.allies);
                trailPicker("Enemies", trails.enemies);
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }

        ImGui::SetNextItemWidth(95.0f);
        ImGui::InputFloat("Text Cull Distance", &sharedConfig.textCullDistance, 0.4f, 0.8f, "%.1fm");
        sharedConfig.textCullDistance = std::clamp(sharedConfig.textCullDistance, 0.0f, 999.9f);
    }

    ImGui::EndChild();

    if (!contentOnly)
        ImGui::End();
}

// Visuals

void GUI::renderVisualsWindow(bool contentOnly) noexcept {
    if (!contentOnly) {
        if (!window.visuals)
            return;
        ImGui::SetNextWindowSize({ 680.0f, 0.0f });
        ImGui::Begin("Visuals", &window.visuals, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
                     | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    }
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 280.0f);
    ImGui::Checkbox("Disable Post Processing", &config->visualsConfig.disablePostProcessing);
    ImGui::Checkbox("Inverse Ragdoll Gravity", &config->visualsConfig.inverseRagdollGravity);
    ImGui::Checkbox("No Fog", &config->visualsConfig.noFog);
    ImGui::Checkbox("No 3D Sky", &config->visualsConfig.no3dSky);
    ImGui::Checkbox("No Aim Punch", &config->visualsConfig.noAimPunch);
    ImGui::Checkbox("No View Punch", &config->visualsConfig.noViewPunch);
    ImGui::Checkbox("No Hands", &config->visualsConfig.noHands);
    ImGui::Checkbox("No Sleeves", &config->visualsConfig.noSleeves);
    ImGui::Checkbox("No Weapons", &config->visualsConfig.noWeapons);
    ImGui::Checkbox("No Smoke", &config->visualsConfig.noSmoke);
    ImGui::Checkbox("No Blur", &config->visualsConfig.noBlur);
    ImGui::Checkbox("No Scope Overlay", &config->visualsConfig.noScopeOverlay);
    ImGui::Checkbox("No Grass", &config->visualsConfig.noGrass);
    ImGui::Checkbox("No Shadows", &config->visualsConfig.noShadows);
    ImGui::Checkbox("Wireframe Smoke", &config->visualsConfig.wireframeSmoke);
    ImGui::NextColumn();
    ImGui::Checkbox("Zoom", &config->visualsConfig.zoom);
    ImGui::SameLine();
    ImGui::PushID("Zoom Key");
    ImGui::hotkey("", config->visualsConfig.zoomKey);
    ImGui::PopID();
    ImGui::Checkbox("Thirdperson", &config->visualsConfig.thirdperson);
    ImGui::SameLine();
    ImGui::PushID("Thirdperson Key");
    ImGui::hotkey("", config->visualsConfig.thirdpersonKey);
    ImGui::PopID();
    ImGui::PushItemWidth(290.0f);
    ImGui::PushID(0);
    ImGui::SliderInt("", &config->visualsConfig.thirdpersonDistance, 0, 1000, "Distance: %d");
    ImGui::PopID();
    ImGui::PushID(1);
    ImGui::SliderInt("", &config->visualsConfig.viewmodelFov, -60, 60, "Viewmodel FOV: %d");
    ImGui::PopID();
    ImGui::PushID(2);
    ImGui::SliderInt("", &config->visualsConfig.fov, -60, 60, "FOV: %d");
    ImGui::PopID();
    ImGui::PushID(3);
    ImGui::SliderInt("", &config->visualsConfig.farZ, 0, 2000, "Far Z: %d");
    ImGui::PopID();
    ImGui::PushID(4);
    ImGui::SliderInt("", &config->visualsConfig.flashReduction, 0, 100, "Flash Reduction: %d%%");
    ImGui::PopID();
    ImGui::PushID(5);
    ImGui::SliderFloat("", &config->visualsConfig.brightness, 0.0f, 1.0f, "Brightness: %.2f");
    ImGui::PopID();
    ImGui::PopItemWidth();
    ImGui::Combo("Skybox", &config->visualsConfig.skybox, Visuals::skyboxList.data(), Visuals::skyboxList.size());
    ImGuiCustom::colorPicker("World Color", config->visualsConfig.world);
    ImGuiCustom::colorPicker("Sky Color", config->visualsConfig.sky);
    ImGui::Checkbox("Deagle spinner", &config->visualsConfig.deagleSpinner);
    ImGui::Combo("Screen Effect", &config->visualsConfig.screenEffect, "None\0Drone Cam\0Drone Cam with Noise\0Underwater\0Healthboost\0Dangerzone\0");
    ImGui::Combo("Hit Effect", &config->visualsConfig.hitEffect, "None\0Drone Cam\0Drone Cam with Noise\0Underwater\0Healthboost\0Dangerzone\0");
    ImGui::SliderFloat("Hit Effect Time", &config->visualsConfig.hitEffectTime, 0.1f, 1.5f, "%.2fs");
    ImGui::Combo("Hit Marker", &config->visualsConfig.hitMarker, "None\0Default (Cross)\0");
    ImGui::SliderFloat("Hit Marker Time", &config->visualsConfig.hitMarkerTime, 0.1f, 1.5f, "%.2fs");
    ImGuiCustom::colorPicker("Bullet Tracers", config->visualsConfig.bulletTracers.asColor4().color.data(), &config->visualsConfig.bulletTracers.asColor4().color[3], nullptr, nullptr, &config->visualsConfig.bulletTracers.enabled);
    ImGuiCustom::colorPicker("Molotov Hull", config->visualsConfig.molotovHull);

    ImGui::Checkbox("Color Correction", &config->visualsConfig.colorCorrection.enabled);
    ImGui::SameLine();

    if (bool ccPopup = ImGui::Button("Edit"))
        ImGui::OpenPopup("##popup");

    if (ImGui::BeginPopup("##popup")) {
        ImGui::VSliderFloat("##1", { 40.0f, 160.0f }, &config->visualsConfig.colorCorrection.blue, 0.0f, 1.0f, "Blue\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##2", { 40.0f, 160.0f }, &config->visualsConfig.colorCorrection.red, 0.0f, 1.0f, "Red\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##3", { 40.0f, 160.0f }, &config->visualsConfig.colorCorrection.mono, 0.0f, 1.0f, "Mono\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##4", { 40.0f, 160.0f }, &config->visualsConfig.colorCorrection.saturation, 0.0f, 1.0f, "Sat\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##5", { 40.0f, 160.0f }, &config->visualsConfig.colorCorrection.ghost, 0.0f, 1.0f, "Ghost\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##6", { 40.0f, 160.0f }, &config->visualsConfig.colorCorrection.green, 0.0f, 1.0f, "Green\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##7", { 40.0f, 160.0f }, &config->visualsConfig.colorCorrection.yellow, 0.0f, 1.0f, "Yellow\n%.3f"); ImGui::SameLine();
        ImGui::EndPopup();
    }
    ImGui::Columns(1);

    if (!contentOnly)
        ImGui::End();
}

// INVENTORY CHANGER

// I couldn't put the Inventory Changer GUI code here because of linker errors.

// STYLE

void GUI::renderStyleWindow(bool contentOnly) noexcept {
    if (!contentOnly) {
        if (!window.style)
            return;

        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Style", &window.style, windowFlags);
    }

    ImGuiStyle& style = ImGui::GetStyle();

    // Window rounding slider flota here

    ImGuiCustom::colorPicker("Menu Colour", (float*)&config->styleConfig.menuColour.asColor3().color);
    ImGui::PushItemWidth(150.0f);
    if (ImGui::Combo("Menu Colours", &config->styleConfig.menuColours, "Main\0Dark\0Light\0Custom\0"))
        updateColors();
    ImGui::PopItemWidth();

    if (config->styleConfig.menuColours == 3) {
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            if (i && i & 3) ImGui::SameLine(220.0f * (i & 3));

            ImGuiCustom::colorPicker(ImGui::GetStyleColorName(i), (float*)&style.Colors[i], &style.Colors[i].w);
        }
    }

    if (!contentOnly)
        ImGui::End();
}

// MISC

void GUI::renderMiscWindow(bool contentOnly) noexcept {
    if (!contentOnly) {
        if (!window.misc)
            return;
        ImGui::SetNextWindowSize({ 580.0f, 0.0f });
        ImGui::Begin("Misc", &window.misc, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
                     | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    }
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 230.0f);
    ImGui::hotkey("Menu Key", config->miscConfig.menuKey);
    ImGui::Checkbox("Anti AFK", &config->miscConfig.antiAfk);
    ImGui::Checkbox("Bunny Hop", &config->miscConfig.bunnyHop);
    ImGui::Checkbox("Fast Duck", &config->miscConfig.fastDuck);
    ImGui::Checkbox("Moon Walk", &config->miscConfig.moonWalk);
    ImGui::Checkbox("Edge Jump", &config->miscConfig.edgeJump);
    ImGui::SameLine();
    ImGui::PushID("Edge Jump Key");
    ImGui::hotkey("", config->miscConfig.edgeJumpKey);
    ImGui::PopID();
    ImGui::Checkbox("Slow Walk", &config->miscConfig.slowWalk);
    ImGui::SameLine();
    ImGui::PushID("Slow Walk Key");
    ImGui::hotkey("", config->miscConfig.slowWalkKey);
    ImGui::PopID();
    ImGuiCustom::colorPicker("Noscope Crosshair", config->miscConfig.noscopeCrosshair);
    ImGuiCustom::colorPicker("Recoil Crosshair", config->miscConfig.recoilCrosshair);
    ImGui::Checkbox("Auto Accept", &config->miscConfig.autoAccept);
    ImGui::Checkbox("Infinite Radar", &config->miscConfig.infiniteRadar);
    ImGui::Checkbox("Reveal Ranks", &config->miscConfig.revealRanks);
    ImGui::Checkbox("Reveal Money", &config->miscConfig.revealMoney);
    ImGui::Checkbox("Reveal Suspect", &config->miscConfig.revealSuspect);
    ImGui::Checkbox("Reveal Votes", &config->miscConfig.revealVotes);

    ImGui::Checkbox("Spectator List", &config->miscConfig.spectatorList.enabled);
    ImGui::SameLine();

    ImGui::PushID("Spectator List");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Checkbox("No Title Bar", &config->miscConfig.spectatorList.noTitleBar);
        ImGui::EndPopup();
    }
    ImGui::PopID();

    ImGui::Checkbox("Purchase List", &config->miscConfig.purchaseList.enabled);
    ImGui::SameLine();

    ImGui::PushID("Purchase List");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::SetNextItemWidth(75.0f);
        ImGui::Combo("Mode", &config->miscConfig.purchaseList.mode, "Details\0Summary\0");
        ImGui::Checkbox("Only During Freeze Time", &config->miscConfig.purchaseList.onlyDuringFreezeTime);
        ImGui::Checkbox("Show Prices", &config->miscConfig.purchaseList.showPrices);
        ImGui::Checkbox("No Title Bar", &config->miscConfig.purchaseList.noTitleBar);
        ImGui::EndPopup();
    }
    ImGui::PopID();

    ImGui::Checkbox("Team Damage List", &config->miscConfig.teamDamageList.enabled);
    ImGui::SameLine();

    ImGui::PushID("Team Damage List");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Text("uwu");
        ImGui::EndPopup();
    }
    ImGui::PopID();

    ImGui::Checkbox("Watermark", &config->miscConfig.watermark.enabled);
    ImGuiCustom::colorPicker("Offscreen Enemies", config->miscConfig.offscreenEnemies.asColor4(), &config->miscConfig.offscreenEnemies.enabled);
    ImGui::SameLine();
    ImGui::PushID("Offscreen Enemies");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Checkbox("Health Bar", &config->miscConfig.offscreenEnemies.healthBar.enabled);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(95.0f);
        ImGui::Combo("Type", &config->miscConfig.offscreenEnemies.healthBar.type, "Gradient\0Solid\0Health-Based\0");
        if (config->miscConfig.offscreenEnemies.healthBar.type == HealthBar::Solid) {
            ImGui::SameLine();
            ImGuiCustom::colorPicker("", config->miscConfig.offscreenEnemies.healthBar.asColor4());
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::Checkbox("Fix Animation LOD", &config->miscConfig.fixAnimationLOD);
    ImGui::Checkbox("Fix Bone Matrix", &config->miscConfig.fixBoneMatrix);
    ImGui::Checkbox("Fix Movement", &config->miscConfig.fixMovement);
    ImGui::Checkbox("Disable Model Occlusion", &config->miscConfig.disableModelOcclusion);
    ImGui::NextColumn();
    ImGui::Checkbox("Animated Clan Tag", &config->miscConfig.animatedClanTag);
    ImGui::Checkbox("Custom Clantag", &config->miscConfig.customClanTag);
    ImGui::SameLine();
    ImGui::PushItemWidth(120.0f);
    ImGui::PushID(0);

    if (ImGui::InputText("", config->miscConfig.clanTag, sizeof(config->miscConfig.clanTag)))
        Misc::updateClanTag(true);

    ImGui::PopID();
    ImGui::Checkbox("Kill Message", &config->miscConfig.killMessage);
    ImGui::SameLine();
    ImGui::PushItemWidth(120.0f);
    ImGui::PushID(1);
    ImGui::InputText("", &config->miscConfig.killMessageString);
    ImGui::PopID();
    ImGui::Checkbox("Name Stealer", &config->miscConfig.nameStealer);
    ImGui::PushID(3);
    ImGui::SetNextItemWidth(100.0f);

    ImGui::Checkbox("Custom Player Name", &config->miscConfig.customPlayerName);
    ImGui::SameLine();
    if (ImGui::InputText("", &config->miscConfig.playerName, sizeof(&config->miscConfig.playerName)))
        Misc::updatePlayerName();

    ImGui::PopID();
    ImGui::Checkbox("Fast Stop", &config->miscConfig.fastStop);
    ImGuiCustom::colorPicker("Bomb Timer", config->miscConfig.bombTimer);
    ImGui::Checkbox("Prepare Revolver", &config->miscConfig.prepareRevolver);
    ImGui::Combo("Hit Sound", &config->miscConfig.hitSound, "None\0Metal\0Gamesense\0Bell\0Glass\0Custom\0");
    if (config->miscConfig.hitSound == 5) {
        ImGui::InputText("Hit Sound filename", &config->miscConfig.customHitSound);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Audio file must be put in csgo/sound/ directory!");
    }
    ImGui::PushID(5);
    ImGui::Combo("Kill Sound", &config->miscConfig.killSound, "None\0Metal\0Gamesense\0Bell\0Glass\0Custom\0");
    if (config->miscConfig.killSound == 5) {
        ImGui::InputText("Kill Sound filename", &config->miscConfig.customKillSound);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Audio file must be put in csgo/sound/ directory!");
    }
    ImGui::PopID();
    ImGui::SetNextItemWidth(90.0f);
    ImGui::Checkbox("Grenade Prediction", &config->miscConfig.nadePredict);
    ImGui::Checkbox("Infinite Tablet", &config->miscConfig.infiniteTablet);
    ImGui::SetNextItemWidth(120.0f);
    ImGui::SliderFloat("Max Angle Delta", &config->miscConfig.maxAngleDelta, 0.0f, 255.0f, "%.2f");
    ImGui::Checkbox("Preserve Killfeed", &config->miscConfig.preserveKillfeed.enabled);
    ImGui::SameLine();

    ImGui::PushID("Preserve Killfeed");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Checkbox("Only Headshots", &config->miscConfig.preserveKillfeed.onlyHeadshots);
        ImGui::EndPopup();
    }
    ImGui::PopID();

    ImGui::Checkbox("Reportbot", &config->miscConfig.reportbot.enabled);
    ImGui::SameLine();
    ImGui::PushID("Reportbot");

    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::PushItemWidth(80.0f);
        ImGui::Combo("Target", &config->miscConfig.reportbot.target, "Enemies\0Allies\0All\0");
        ImGui::InputInt("Delay (s)", &config->miscConfig.reportbot.delay);
        config->miscConfig.reportbot.delay = (std::max)(config->miscConfig.reportbot.delay, 1);
        ImGui::InputInt("Rounds", &config->miscConfig.reportbot.rounds);
        config->miscConfig.reportbot.rounds = (std::max)(config->miscConfig.reportbot.rounds, 1);
        ImGui::PopItemWidth();
        ImGui::Checkbox("Abusive Communications", &config->miscConfig.reportbot.textAbuse);
        ImGui::Checkbox("Griefing", &config->miscConfig.reportbot.griefing);
        ImGui::Checkbox("Wall Hacking", &config->miscConfig.reportbot.wallhack);
        ImGui::Checkbox("Aim Hacking", &config->miscConfig.reportbot.aimbot);
        ImGui::Checkbox("Other Hacking", &config->miscConfig.reportbot.other);
        if (ImGui::Button("Reset"))
            Misc::resetReportbot();
        ImGui::EndPopup();
    }
    ImGui::PopID();

    if (ImGui::Button("Unhook"))
        hooks->uninstall();

    ImGui::Columns(1);
    if (!contentOnly)
        ImGui::End();
}

// CONFIG

void GUI::renderConfigWindow(bool contentOnly) noexcept {
    if (!contentOnly) {
        if (!window.config)
            return;
        ImGui::SetNextWindowSize({ 320.0f, 0.0f });
        if (!ImGui::Begin("Config", &window.config, windowFlags)) {
            ImGui::End();
            return;
        }
    }

    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 170.0f);

    static bool incrementalLoad = false;
    ImGui::Checkbox("Incremental Load", &incrementalLoad);

    ImGui::PushItemWidth(160.0f);

    auto& configItems = config->getConfigs();
    static int currentConfig = -1;

    static std::u8string buffer;

    timeToNextConfigRefresh -= ImGui::GetIO().DeltaTime;
    if (timeToNextConfigRefresh <= 0.0f) {
        config->listConfigs();
        if (const auto it = std::find(configItems.begin(), configItems.end(), buffer); it != configItems.end())
            currentConfig = std::distance(configItems.begin(), it);
        timeToNextConfigRefresh = 0.1f;
    }

    if (static_cast<std::size_t>(currentConfig) >= configItems.size())
        currentConfig = -1;

    if (ImGui::ListBox("", &currentConfig, [](void* data, int idx, const char** out_text) {
        auto& vector = *static_cast<std::vector<std::u8string>*>(data);
        *out_text = (const char*)vector[idx].c_str();
        return true;
        }, &configItems, configItems.size(), 5) && currentConfig != -1)
        buffer = configItems[currentConfig];

        ImGui::PushID(0);
        if (ImGui::InputTextWithHint("", "Config Name", &buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (currentConfig != -1)
                config->rename(currentConfig, buffer);
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::PushItemWidth(100.0f);

        if (ImGui::Button("Open Config Directory"))
            config->openConfigDir();

        if (ImGui::Button("Create Config", { 100.0f, 25.0f }))
            config->add(buffer.c_str());

        if (ImGui::Button("Reset Config", { 100.0f, 25.0f }))
            ImGui::OpenPopup("Config to reset...");

        if (ImGui::BeginPopup("Config to reset...")) {
            static constexpr const char* names[]{ "Whole", "Aimbot", "Triggerbot", "Backtrack", "Anti Aim", "Glow", "Chams", "ESP", "Visuals", "Inventory Changer", "Misc", "Style" };
            for (int i = 0; i < IM_ARRAYSIZE(names); i++) {
                if (i == 1) ImGui::Separator();

                if (ImGui::Selectable(names[i])) {
                    switch (i) {
                        case 0: config->reset(); updateColors(); Misc::updateClanTag(true); InventoryChanger::scheduleHudUpdate(); break;
                        case 1: config->aimbotConfig = {}; break;
                        case 2: config->triggerbotConfig = {}; break;
                        case 3: config->backtrackConfig = {}; break;
                            //case 4: config->antiAimConfig = {}; break; TODO
                        case 5: config->glowConfig = {}; break;
                        case 6: config->chamsConfig = {}; break;
                        case 7: config->espConfig = {}; break;
                        case 8: config->visualsConfig = {}; break;
                        case 9: InventoryChanger::resetConfig(); InventoryChanger::scheduleHudUpdate(); break;
                        case 10: config->miscConfig = {}; Misc::updateClanTag(true); break;
                        case 11: config->styleConfig = {}; updateColors(); break;
                    }
                }
            }
            ImGui::EndPopup();
        }
        if (currentConfig != -1) {
            if (ImGui::Button("Load Selected", { 100.0f, 25.0f })) {
                config->load(currentConfig, incrementalLoad);
                updateColors();
                InventoryChanger::scheduleHudUpdate();
                Misc::updateClanTag(true);
            }
            if (ImGui::Button("Save Selected", { 100.0f, 25.0f }))
                config->save(currentConfig);
            if (ImGui::Button("Delete Selected", { 100.0f, 25.0f })) {
                config->remove(currentConfig);

                if (static_cast<std::size_t>(currentConfig) < configItems.size())
                    buffer = configItems[currentConfig];
                else
                    buffer.clear();
            }
        }
        ImGui::Columns(1);
        if (!contentOnly)
            ImGui::End();
}
