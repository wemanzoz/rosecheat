#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <system_error>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#include <ShlObj.h>
#endif

#include "nlohmann/json.hpp"

#include "imgui/imgui.h"

#include "Config.h"
#include "Helpers.h"
#include "InventoryChanger/InventoryChanger.h"

#ifdef _WIN32
int CALLBACK fontCallback(const LOGFONTW* lpelfe, const TEXTMETRICW*, DWORD, LPARAM lParam) {
    const wchar_t* const fontName = reinterpret_cast<const ENUMLOGFONTEXW*>(lpelfe)->elfFullName;

    if (fontName[0] == L'@')
        return TRUE;

    if (HFONT font = CreateFontW(0, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, fontName)) {
        DWORD fontData = GDI_ERROR;

        if (HDC hdc = CreateCompatibleDC(nullptr)) {
            SelectObject(hdc, font);
            // Do not use TTC fonts as we only support TTF fonts
            fontData = GetFontData(hdc, 'fctt', 0, NULL, 0);
            DeleteDC(hdc);
        }
        DeleteObject(font);

        if (fontData == GDI_ERROR) {
            if (char buff[1024]; WideCharToMultiByte(CP_UTF8, 0, fontName, -1, buff, sizeof(buff), nullptr, nullptr))
                reinterpret_cast<std::vector<std::string>*>(lParam)->emplace_back(buff);
        }
    }
    return TRUE;
}
#endif

void removeEmptyObjects(json& j) noexcept {
    for (auto it = j.begin(); it != j.end();) {
        auto& val = it.value();
        if (val.is_object() || val.is_array())
            removeEmptyObjects(val);
        if (val.empty() && !j.is_array())
            it = j.erase(it);
        else
            ++it;
    }
}

[[nodiscard]] static std::filesystem::path buildConfigsFolderPath() noexcept {
    std::filesystem::path path;
#ifdef _WIN32
    if (PWSTR pathToDocuments; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathToDocuments))) {
        path = pathToDocuments;
        CoTaskMemFree(pathToDocuments);
    }
#else
    if (const char* homeDir = getenv("HOME"))
        path = homeDir;
#endif

    path /= "rosecheat";
    return path;
}

Config::Config() noexcept : path{ buildConfigsFolderPath() } {
    listConfigs();

    load(u8"default.json", false);

#ifdef _WIN32
    LOGFONTW logfont;
    logfont.lfCharSet = ANSI_CHARSET;
    logfont.lfPitchAndFamily = DEFAULT_PITCH;
    logfont.lfFaceName[0] = L'\0';

    EnumFontFamiliesExW(GetDC(nullptr), &logfont, fontCallback, (LPARAM)&systemFonts, 0);
#endif

    std::sort(std::next(systemFonts.begin()), systemFonts.end());
}

static void to_json(json& j, const ImVec2& o, const ImVec2& dummy = {}) {
    WRITE("X", x);
    WRITE("Y", y);
}

// AIMBOT

static void from_json(const json& j, Config::Aimbot& a) {
    read(j, "Enabled", a.enabled);
    read(j, "Aimlock", a.aimlock);
    read(j, "Silent", a.silent);
    read(j, "Friendly Fire", a.friendlyFire);
    read(j, "Visible Only", a.visibleOnly);
    read(j, "Scoped Only", a.scopedOnly);
    read(j, "Ignore Flash", a.ignoreFlash);
    read(j, "Ignore Smoke", a.ignoreSmoke);
    read(j, "Auto Shot", a.autoShot);
    read(j, "Auto Scope", a.autoScope);
    read(j, "FOV", a.fov);
    read(j, "Smooth", a.smooth);
    read(j, "Bone", a.bone);
    read(j, "Max Aim Inaccuracy", a.maxAimInaccuracy);
    read(j, "Max Shot Inaccuracy", a.maxShotInaccuracy);
    read(j, "Minimum Damage", a.minDamage);
    read(j, "Killshot", a.killshot);
    read(j, "Between Shots", a.betweenShots);
}

static void to_json(json& j, const Config::Aimbot& o, const Config::Aimbot& dummy = {}) {
    WRITE("Enabled", enabled);
    WRITE("Aimlock", aimlock);
    WRITE("Silent", silent);
    WRITE("Friendly Fire", friendlyFire);
    WRITE("Visible Only", visibleOnly);
    WRITE("Scoped Only", scopedOnly);
    WRITE("Ignore Flash", ignoreFlash);
    WRITE("Ignore Smoke", ignoreSmoke);
    WRITE("Auto Shot", autoShot);
    WRITE("Auto Scope", autoScope);
    WRITE("FOV", fov);
    WRITE("Smooth", smooth);
    WRITE("Bone", bone);
    WRITE("Max Aim Inaccuracy", maxAimInaccuracy);
    WRITE("Max Shot Inaccuracy", maxShotInaccuracy);
    WRITE("Minimum Damage", minDamage);
    WRITE("Killshot", killshot);
    WRITE("Between Shots", betweenShots);
}

// TRIGGERBOT

static void from_json(const json& j, Config::Triggerbot& t) {
    read(j, "Enabled", t.enabled);
    read(j, "Friendly Fire", t.friendlyFire);
    read(j, "Scoped Only", t.scopedOnly);
    read(j, "Ignore Flash", t.ignoreFlash);
    read(j, "Ignore Smoke", t.ignoreSmoke);
    read(j, "Hitgroup", t.hitgroup);
    read(j, "Shot Delay", t.shotDelay);
    read(j, "Minimum Damage", t.minDamage);
    read(j, "Killshot", t.killshot);
    read(j, "Burst Time", t.burstTime);
}

static void to_json(json& j, const Config::Triggerbot& o, const Config::Triggerbot& dummy = {}) {
    WRITE("Enabled", enabled);
    WRITE("Friendly Fire", friendlyFire);
    WRITE("Scoped Only", scopedOnly);
    WRITE("Ignore Flash", ignoreFlash);
    WRITE("Ignore Smoke", ignoreSmoke);
    WRITE("Hitgroup", hitgroup);
    WRITE("Shot Delay", shotDelay);
    WRITE("Minimum Damage", minDamage);
    WRITE("Killshot", killshot);
    WRITE("Burst Time", burstTime);
}

// BACKTRACK

static void from_json(const json& j, Config::BacktrackConfig& b) {
    read(j, "Enabled", b.enabled);
    read(j, "Ignore Smoke", b.ignoreSmoke);
    read(j, "Recoil Based FOV", b.recoilBasedFov);
    read(j, "Limit", b.timeLimit);
}

static void to_json(json& j, const Config::BacktrackConfig& o, const Config::BacktrackConfig& dummy = {}) {
    WRITE("Enabled", enabled);
    WRITE("Ignore Smoke", ignoreSmoke);
    WRITE("Recoil Based FOV", recoilBasedFov);
    WRITE("Limit", timeLimit);
}

// ANTI AIM TODO

//static void from_json(const json& j, Config::AntiAimConfig& a) {
//    read(j, "Enabled", a.enabled);
//}
//
//static void to_json(json& j, const Config::AntiAimConfig& o, const Config::AntiAimConfig& dummy = {}) {
//    WRITE("Enabled", enabled);
//}

// GLOW

static void to_json(json& j, const Config::GlowItem& o, const Config::GlowItem& dummy = {}) {
    to_json(j, static_cast<const Color4&>(o), dummy);
    WRITE("Enabled", enabled);
    WRITE("Health based", healthBased);
    WRITE("Style", style);
}

static void to_json(json& j, const Config::PlayerGlow& o, const Config::PlayerGlow& dummy = {}) {
    WRITE("All", all);
    WRITE("Visible", visible);
    WRITE("Occluded", occluded);
}

static void from_json(const json& j, Config::GlowItem& g) {
    from_json(j, static_cast<Color4&>(g));

    read(j, "Enabled", g.enabled);
    read(j, "Health based", g.healthBased);
    read(j, "Style", g.style);
}

static void from_json(const json& j, Config::PlayerGlow& g) {
    read<value_t::object>(j, "All", g.all);
    read<value_t::object>(j, "Visible", g.visible);
    read<value_t::object>(j, "Occluded", g.occluded);
}

// CHAMS

static void from_json(const json& j, Config::Chams::Material& m) {
    from_json(j, static_cast<Color4&>(m));

    read(j, "Enabled", m.enabled);
    read(j, "Health Based", m.healthBased);
    read(j, "Blinking", m.blinking);
    read(j, "Wireframe", m.wireframe);
    read(j, "Cover", m.cover);
    read(j, "Ignore-Z", m.ignorez);
    read(j, "Material", m.material);
}

static void from_json(const json& j, Config::Chams& c) {
    read_array_opt(j, "Materials", c.materials);
}

static void to_json(json& j, const Config::Chams::Material& o) {
    const Config::Chams::Material dummy;

    to_json(j, static_cast<const Color4&>(o), dummy);
    WRITE("Enabled", enabled);
    WRITE("Health Based", healthBased);
    WRITE("Blinking", blinking);
    WRITE("Wireframe", wireframe);
    WRITE("Cover", cover);
    WRITE("Ignore-Z", ignorez);
    WRITE("Material", material);
}

static void to_json(json& j, const Config::Chams& o) {
    j["Materials"] = o.materials;
}

// ESP

static void from_json(const json& j, Snapline& s) {
    from_json(j, static_cast<ColorToggleThickness&>(s));

    read(j, "Type", s.type);
}

static void to_json(json& j, const Snapline& o, const Snapline& dummy = {}) {
    to_json(j, static_cast<const ColorToggleThickness&>(o), dummy);
    WRITE("Type", type);
}

static void from_json(const json& j, ColorToggleRounding& ctr) {
    from_json(j, static_cast<ColorToggle&>(ctr));

    read(j, "Rounding", ctr.rounding);
}

static void to_json(json& j, const ColorToggleRounding& o, const ColorToggleRounding& dummy = {}) {
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Rounding", rounding);
}

static void to_json(json& j, const ColorToggleThicknessRounding& o, const ColorToggleThicknessRounding& dummy = {}) {
    to_json(j, static_cast<const ColorToggleRounding&>(o), dummy);
    WRITE("Thickness", thickness);
}

static void from_json(const json& j, Box& b) {
    from_json(j, static_cast<ColorToggleRounding&>(b));

    read(j, "Type", b.type);
    read(j, "Scale", b.scale);
    read<value_t::object>(j, "Fill", b.fill);
}

static void to_json(json& j, const Box& o, const Box& dummy = {}) {
    to_json(j, static_cast<const ColorToggleRounding&>(o), dummy);
    WRITE("Type", type);
    WRITE("Scale", scale);
    WRITE("Fill", fill);
}
static void from_json(const json& j, Shared& s) {
    read(j, "Enabled", s.enabled);
    read<value_t::object>(j, "Font", s.font);
    read<value_t::object>(j, "Snapline", s.snapline);
    read<value_t::object>(j, "Box", s.box);
    read<value_t::object>(j, "Name", s.name);
    read(j, "Text Cull Distance", s.textCullDistance);
}

static void to_json(json& j, const Shared& o, const Shared& dummy = {}) {
    WRITE("Enabled", enabled);
    WRITE("Font", font);
    WRITE("Snapline", snapline);
    WRITE("Box", box);
    WRITE("Name", name);
    WRITE("Text Cull Distance", textCullDistance);
}

static void from_json(const json& j, Font& f) {
    read<value_t::string>(j, "Name", f.name);

    if (!f.name.empty())
        config->scheduleFontLoad(f.name);

    if (const auto it = std::ranges::find(config->getSystemFonts(), f.name); it != config->getSystemFonts().end())
        f.index = std::distance(config->getSystemFonts().begin(), it);
    else
        f.index = 0;
}

static void to_json(json& j, const Font& o, const Font& dummy = {}) {
    WRITE("Name", name);
}

static void from_json(const json& j, Weapon& w) {
    from_json(j, static_cast<Shared&>(w));

    read<value_t::object>(j, "Ammo", w.ammo);
}

static void to_json(json& j, const Weapon& o, const Weapon& dummy = {}) {
    to_json(j, static_cast<const Shared&>(o), dummy);
    WRITE("Ammo", ammo);
}

static void from_json(const json& j, Projectile& p) {
    from_json(j, static_cast<Shared&>(p));

    read<value_t::object>(j, "Trails", p.trails);
}

static void to_json(json& j, const Projectile& o, const Projectile& dummy = {}) {
    j = static_cast<const Shared&>(o);

    WRITE("Trails", trails);
}

static void from_json(const json& j, Trail& t) {
    from_json(j, static_cast<ColorToggleThickness&>(t));

    read(j, "Type", t.type);
    read(j, "Time", t.time);
}

static void to_json(json& j, const Trail& o, const Trail& dummy = {}) {
    to_json(j, static_cast<const ColorToggleThickness&>(o), dummy);
    WRITE("Type", type);
    WRITE("Time", time);
}

static void from_json(const json& j, Trails& t) {
    read(j, "Enabled", t.enabled);
    read<value_t::object>(j, "Local Player", t.localPlayer);
    read<value_t::object>(j, "Allies", t.allies);
    read<value_t::object>(j, "Enemies", t.enemies);
}

static void to_json(json& j, const Trails& o, const Trails& dummy = {}) {
    WRITE("Enabled", enabled);
    WRITE("Local Player", localPlayer);
    WRITE("Allies", allies);
    WRITE("Enemies", enemies);
}

static void from_json(const json& j, Player& p) {
    from_json(j, static_cast<Shared&>(p));

    read<value_t::object>(j, "Weapon", p.weapon);
    read<value_t::object>(j, "Flash Duration", p.flashDuration);
    read(j, "Audible Only", p.audibleOnly);
    read(j, "Spotted Only", p.spottedOnly);
    read<value_t::object>(j, "Health Bar", p.healthBar);
    read<value_t::object>(j, "Skeleton", p.skeleton);
    read<value_t::object>(j, "Head Box", p.headBox);
}

static void to_json(json& j, const Player& o, const Player& dummy = {}) {
    to_json(j, static_cast<const Shared&>(o), dummy);
    WRITE("Weapon", weapon);
    WRITE("Flash Duration", flashDuration);
    WRITE("Audible Only", audibleOnly);
    WRITE("Spotted Only", spottedOnly);
    WRITE("Health Bar", healthBar);
    WRITE("Skeleton", skeleton);
    WRITE("Head Box", headBox);
}

static void from_json(const json& j, Config::ESP& e) {
    read(j, "Toggle Key", e.toggleKey);
    read(j, "Hold Key", e.holdKey);
    read(j, "Allies", e.allies);
    read(j, "Enemies", e.enemies);
    read(j, "Weapons", e.weapons);
    read(j, "Projectiles", e.projectiles);
    read(j, "Loot Crates", e.lootCrates);
    read(j, "Other Entities", e.otherEntities);
}

static void to_json(json& j, const Config::ESP& o, const Config::ESP& dummy = {}) {
    WRITE("Toggle Key", toggleKey);
    WRITE("Hold Key", holdKey);
    j["Allies"] = o.allies;
    j["Enemies"] = o.enemies;
    j["Weapons"] = o.weapons;
    j["Projectiles"] = o.projectiles;
    j["Loot Crates"] = o.lootCrates;
    j["Other Entities"] = o.otherEntities;
}

// VISUALS

static void from_json(const json& j, Config::VisualsConfig::ColorCorrection& c) {
    read(j, "Enabled", c.enabled);
    read(j, "Blue", c.blue);
    read(j, "Red", c.red);
    read(j, "Mono", c.mono);
    read(j, "Saturation", c.saturation);
    read(j, "Ghost", c.ghost);
    read(j, "Green", c.green);
    read(j, "Yellow", c.yellow);
}

static void from_json(const json& j, Config::BulletTracers& o) {
    from_json(j, static_cast<ColorToggle&>(o));
}

static void from_json(const json& j, Config::VisualsConfig& v) {
    read(j, "Disable Post Processing", v.disablePostProcessing);
    read(j, "Inverse Ragdoll Gravity", v.inverseRagdollGravity);
    read(j, "No Fog", v.noFog);
    read(j, "No 3D Sky", v.no3dSky);
    read(j, "No Aim Punch", v.noAimPunch);
    read(j, "No View Punch", v.noViewPunch);
    read(j, "No Hands", v.noHands);
    read(j, "No Sleeves", v.noSleeves);
    read(j, "No Weapons", v.noWeapons);
    read(j, "No Smoke", v.noSmoke);
    read(j, "No Blur", v.noBlur);
    read(j, "No Scope Overlay", v.noScopeOverlay);
    read(j, "No Grass", v.noGrass);
    read(j, "No Shadows", v.noShadows);
    read(j, "Wireframe Smoke", v.wireframeSmoke);
    read(j, "Zoom", v.zoom);
    read(j, "Zoom Key", v.zoomKey);
    read(j, "Thirdperson", v.thirdperson);
    read(j, "Thirdperson Key", v.thirdpersonKey);
    read(j, "Thirdperson Distance", v.thirdpersonDistance);
    read(j, "Viewmodel FOV", v.viewmodelFov);
    read(j, "FOV", v.fov);
    read(j, "Far Z", v.farZ);
    read(j, "Flash Reduction", v.flashReduction);
    read(j, "Brightness", v.brightness);
    read(j, "Skybox", v.skybox);
    read<value_t::object>(j, "World", v.world);
    read<value_t::object>(j, "Sky", v.sky);
    read(j, "Deagle Spinner", v.deagleSpinner);
    read(j, "Screen Effect", v.screenEffect);
    read(j, "Hit Effect", v.hitEffect);
    read(j, "Hit Effect Time", v.hitEffectTime);
    read(j, "Hit Marker", v.hitMarker);
    read(j, "Hit Marker Time", v.hitMarkerTime);
    read<value_t::object>(j, "Color Correction", v.colorCorrection);
    read<value_t::object>(j, "Bullet Tracers", v.bulletTracers);
    read<value_t::object>(j, "Molotov Hull", v.molotovHull);
}

static void to_json(json& j, const Config::VisualsConfig::ColorCorrection& o, const Config::VisualsConfig::ColorCorrection& dummy) {
    WRITE("Enabled", enabled);
    WRITE("Blue", blue);
    WRITE("Red", red);
    WRITE("Mono", mono);
    WRITE("Saturation", saturation);
    WRITE("Ghost", ghost);
    WRITE("Green", green);
    WRITE("Yellow", yellow);
}

static void to_json(json& j, const Config::BulletTracers& o, const Config::BulletTracers& dummy = {}) {
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
}

static void to_json(json& j, const Config::VisualsConfig& o) {
    const Config::VisualsConfig dummy;

    WRITE("Disable Post Processing", disablePostProcessing);
    WRITE("Inverse Ragdoll Gravity", inverseRagdollGravity);
    WRITE("No Fog", noFog);
    WRITE("No 3D Sky", no3dSky);
    WRITE("No Aim Punch", noAimPunch);
    WRITE("No View Punch", noViewPunch);
    WRITE("No Hands", noHands);
    WRITE("No Sleeves", noSleeves);
    WRITE("No Weapons", noWeapons);
    WRITE("No Smoke", noSmoke);
    WRITE("No Blur", noBlur);
    WRITE("No Scope Overlay", noScopeOverlay);
    WRITE("No Grass", noGrass);
    WRITE("No Shadows", noShadows);
    WRITE("Wireframe Smoke", wireframeSmoke);
    WRITE("Zoom", zoom);
    WRITE("Zoom Key", zoomKey);
    WRITE("Thirdperson", thirdperson);
    WRITE("Thirdperson Key", thirdpersonKey);
    WRITE("Thirdperson Distance", thirdpersonDistance);
    WRITE("Viewmodel FOV", viewmodelFov);
    WRITE("FOV", fov);
    WRITE("Far Z", farZ);
    WRITE("Flash Reduction", flashReduction);
    WRITE("Brightness", brightness);
    WRITE("Skybox", skybox);
    WRITE("World", world);
    WRITE("Sky", sky);
    WRITE("Deagle Spinner", deagleSpinner);
    WRITE("Screen Effect", screenEffect);
    WRITE("Hit Effect", hitEffect);
    WRITE("Hit Effect Time", hitEffectTime);
    WRITE("Hit Marker", hitMarker);
    WRITE("Hit Marker Time", hitMarkerTime);
    WRITE("Color Correction", colorCorrection);
    WRITE("Bullet Tracers", bulletTracers);
    WRITE("Molotov Hull", molotovHull);
}

// MISC

static void from_json(const json& j, ImVec2& v) {
    read(j, "X", v.x);
    read(j, "Y", v.y);
}

static void from_json(const json& j, Config::MiscConfig::SpectatorList& sl) {
    read(j, "Enabled", sl.enabled);
    read(j, "No Title Bar", sl.noTitleBar);
    read<value_t::object>(j, "Pos", sl.pos);
    read<value_t::object>(j, "Size", sl.size);
}

static void from_json(const json& j, Config::MiscConfig::PurchaseList& pl) {
    read(j, "Enabled", pl.enabled);
    read(j, "Only During Freeze Time", pl.onlyDuringFreezeTime);
    read(j, "Show Prices", pl.showPrices);
    read(j, "No Title Bar", pl.noTitleBar);
    read(j, "Mode", pl.mode);
}

static void from_json(const json& j, Config::MiscConfig::TeamDamageList& dl) {
    read(j, "Enabled", dl.enabled);
    read(j, "No Title Bar", dl.noTitleBar);
    read<value_t::object>(j, "Pos", dl.pos);
    read<value_t::object>(j, "Size", dl.size);
}

static void from_json(const json& j, Config::MiscConfig::OffscreenEnemies& o) {
    from_json(j, static_cast<ColorToggle&>(o));

    read<value_t::object>(j, "Health Bar", o.healthBar);
}

static void from_json(const json& j, Config::MiscConfig::Watermark& o) {
    read(j, "Enabled", o.enabled);
}

static void from_json(const json& j, Config::MiscConfig::PreserveKillfeed& o) {
    read(j, "Enabled", o.enabled);
    read(j, "Only Headshots", o.onlyHeadshots);
}

static void from_json(const json& j, Config::MiscConfig& m) {
    read(j, "Menu Key", m.menuKey);
    read(j, "Anti AFK", m.antiAfk);
    read(j, "Bunny Hop", m.bunnyHop);
    read(j, "Custom Clan Tag", m.customClanTag);
    read(j, "Clan Tag", m.clanTag, sizeof(m.clanTag));
    read(j, "Animated Clan Tag", m.animatedClanTag);
    read(j, "Fast Duck", m.fastDuck);
    read(j, "Moon Walk", m.moonWalk);
    read(j, "Edge Jump", m.edgeJump);
    read(j, "Edge Jump Key", m.edgeJumpKey);
    read(j, "Slow Walk", m.slowWalk);
    read(j, "Slow Walk key", m.slowWalkKey);
    read<value_t::object>(j, "Noscope Crosshair", m.noscopeCrosshair);
    read<value_t::object>(j, "Recoil Crosshair", m.recoilCrosshair);
    read(j, "Auto Accept", m.autoAccept);
    read(j, "Infinite Radar", m.infiniteRadar);
    read(j, "Reveal Ranks", m.revealRanks);
    read(j, "Reveal Money", m.revealMoney);
    read(j, "Reveal Suspect", m.revealSuspect);
    read(j, "Reveal Votes", m.revealVotes);
    read<value_t::object>(j, "Spectator List", m.spectatorList);
    read<value_t::object>(j, "Watermark", m.watermark);
    read<value_t::object>(j, "Offscreen Enemies", m.offscreenEnemies);
    read(j, "Fix Animation LOD", m.fixAnimationLOD);
    read(j, "Fix Bone Matrix", m.fixBoneMatrix);
    read(j, "Fix Movement", m.fixMovement);
    read(j, "Disable Model Occlusion", m.disableModelOcclusion);
    read(j, "Kill Message", m.killMessage);
    read<value_t::string>(j, "Kill Message String", m.killMessageString);
    read(j, "Name Stealer", m.nameStealer);
    read<value_t::string>(j, "Player Name", m.playerName);
    read(j, "Fast Stop", m.fastStop);
    read<value_t::object>(j, "Bomb Timer", m.bombTimer);
    read(j, "Prepare Revolver", m.prepareRevolver);
    read(j, "Hit Sound", m.hitSound);
    read(j, "Grenade Predict", m.nadePredict);
    read(j, "Infinite Tablet", m.infiniteTablet);
    read<value_t::string>(j, "Custom Hit Sound", m.customHitSound);
    read(j, "Kill Sound", m.killSound);
    read<value_t::string>(j, "Custom Kill Sound", m.customKillSound);
    read<value_t::object>(j, "Purchase List", m.purchaseList);
    read<value_t::object>(j, "Reportbot", m.reportbot);
    read<value_t::object>(j, "Preserve Killfeed", m.preserveKillfeed);
    read(j, "Max Angle Delta", m.maxAngleDelta);
}

static void from_json(const json& j, Config::MiscConfig::Reportbot& r) {
    read(j, "Enabled", r.enabled);
    read(j, "Target", r.target);
    read(j, "Delay", r.delay);
    read(j, "Rounds", r.rounds);
    read(j, "Abusive Communications", r.textAbuse);
    read(j, "Griefing", r.griefing);
    read(j, "Wall Hacking", r.wallhack);
    read(j, "Aim Hacking", r.aimbot);
    read(j, "Other Hacking", r.other);
}

static void to_json(json& j, const Config::MiscConfig::Reportbot& o, const Config::MiscConfig::Reportbot& dummy = {}) {
    WRITE("Enabled", enabled);
    WRITE("Target", target);
    WRITE("Delay", delay);
    WRITE("Rounds", rounds);
    WRITE("Abusive Communications", textAbuse);
    WRITE("Griefing", griefing);
    WRITE("Wall Hacking", wallhack);
    WRITE("Aim Hacking", aimbot);
    WRITE("Other Hacking", other);
}

static void to_json(json& j, const Config::MiscConfig::SpectatorList& o, const Config::MiscConfig::SpectatorList& dummy = {}) {
    WRITE("Enabled", enabled);
    WRITE("No Title Bar", noTitleBar);

    if (const auto window = ImGui::FindWindowByName("Spectator List")) {
        j["Pos"] = window->Pos;
        j["Size"] = window->SizeFull;
    }
}

static void to_json(json& j, const Config::MiscConfig::PurchaseList& o, const Config::MiscConfig::PurchaseList& dummy = {}) {
    WRITE("Enabled", enabled);
    WRITE("Only During Freeze Time", onlyDuringFreezeTime);
    WRITE("Show Prices", showPrices);
    WRITE("No Title Bar", noTitleBar);
    WRITE("Mode", mode);
}

static void to_json(json& j, const Config::MiscConfig::TeamDamageList& o, const Config::MiscConfig::TeamDamageList& dummy = {}) {
    WRITE("Enabled", enabled);
    WRITE("No Title Bar", noTitleBar);

    if (const auto window = ImGui::FindWindowByName("Team Damage List")) {
        j["Pos"] = window->Pos;
        j["Size"] = window->SizeFull;
    }
}

static void to_json(json& j, const Config::MiscConfig::OffscreenEnemies& o, const Config::MiscConfig::OffscreenEnemies& dummy = {}) {
    to_json(j, static_cast<const ColorToggle&>(o), dummy);

    WRITE("Health Bar", healthBar);
}

static void to_json(json& j, const Config::MiscConfig::Watermark& o, const Config::MiscConfig::Watermark& dummy = {}) {
    WRITE("Enabled", enabled);
}

static void to_json(json& j, const Config::MiscConfig::PreserveKillfeed& o, const Config::MiscConfig::PreserveKillfeed& dummy = {}) {
    WRITE("Enabled", enabled);
    WRITE("Only Headshots", onlyHeadshots);
}

static void to_json(json& j, const Config::MiscConfig& o) {
    const Config::MiscConfig dummy;

    WRITE("Menu Key", menuKey);
    WRITE("Anti AFK", antiAfk);
    WRITE("Bunny Hop", bunnyHop);
    WRITE("Custom Clan Tag", customClanTag);

    if (o.clanTag[0])
        j["Clan Tag"] = o.clanTag;

    WRITE("Animated Clan Tag", animatedClanTag);
    WRITE("Fast Duck", fastDuck);
    WRITE("Moon Walk", moonWalk);
    WRITE("Edge Jump", edgeJump);
    WRITE("Edge Jump Key", edgeJumpKey);
    WRITE("Slow Walk", slowWalk);
    WRITE("Slow Walk Key", slowWalkKey);
    WRITE("Noscope Crosshair", noscopeCrosshair);
    WRITE("Recoil Crosshair", recoilCrosshair);
    WRITE("Auto Accept", autoAccept);
    WRITE("Infinite Radar", infiniteRadar);
    WRITE("Reveal Ranks", revealRanks);
    WRITE("Reveal Money", revealMoney);
    WRITE("Reveal Suspect", revealSuspect);
    WRITE("Reveal Votes", revealVotes);
    WRITE("Spectator List", spectatorList);
    WRITE("Purchase List", purchaseList);
    WRITE("Team Damage List", teamDamageList);
    WRITE("Watermark", watermark);
    WRITE("Offscreen Enemies", offscreenEnemies);
    WRITE("Fix Animation LOD", fixAnimationLOD);
    WRITE("Fix Bone Matrix", fixBoneMatrix);
    WRITE("Fix Movement", fixMovement);
    WRITE("Disable Model Occlusion", disableModelOcclusion);
    WRITE("Kill Message", killMessage);
    WRITE("Kill Message String", killMessageString);
    WRITE("Name Stealer", nameStealer);
    WRITE("Player Name", playerName);
    WRITE("Fast Stop", fastStop);
    WRITE("Bomb Timer", bombTimer);
    WRITE("Prepare Revolver", prepareRevolver);
    WRITE("Hit Sound", hitSound);
    WRITE("Grenade Predict", nadePredict);
    WRITE("Infinite Tablet", infiniteTablet);
    WRITE("Custom Hit Sound", customHitSound);
    WRITE("Kill Sound", killSound);
    WRITE("Custom Kill Sound", customKillSound);
    WRITE("Reportbot", reportbot);
    WRITE("Preserve Killfeed", preserveKillfeed);
    WRITE("Max Angle Delta", maxAngleDelta);
}

// STYLE

static void to_json(json& j, const ImVec4& o) {
    j[0] = o.x;
    j[1] = o.y;
    j[2] = o.z;
    j[3] = o.w;
}

static void from_json(const json& j, Config::Style& s) {
    read(j, "Menu Colour", s.menuColour.asColor3().color);
    read(j, "Menu Colours", s.menuColours);

    if (j.contains("Colours") && j["Colours"].is_object()) {
        const auto& colours = j["Colours"];

        ImGuiStyle& style = ImGui::GetStyle();

        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            if (const char* name = ImGui::GetStyleColorName(i); colours.contains(name)) {
                std::array<float, 4> temp;
                read(colours, name, temp);
                style.Colors[i].x = temp[0];
                style.Colors[i].y = temp[1];
                style.Colors[i].z = temp[2];
                style.Colors[i].w = temp[3];
            }
        }
    }
}

static void to_json(json& j, const Config::Style& o) {
    const Config::Style dummy;

    WRITE("Menu Colour", menuColour);
    WRITE("Menu Colours", menuColours);

    auto& colours = j["Colours"];
    ImGuiStyle& style = ImGui::GetStyle();

    for (int i = 0; i < ImGuiCol_COUNT; i++)
        colours[ImGui::GetStyleColorName(i)] = style.Colors[i];
}

// <-------------------------------------------------->

void Config::save(size_t id) const noexcept {
    json j;

    // AIMBOT

    j["Aimbot"] = aimbotConfig;
    j["Aimbot On Key"] = aimbotOnKey;
    to_json(j["Aimbot Key"], aimbotKey, {});
    j["Aimbot Key Mode"] = aimbotKeyMode;

    // TRIGGERBOT

    j["Triggerbot"] = triggerbotConfig;
    to_json(j["Triggerbot Key"], triggerbotHoldKey, {});

    // BACKTRACK

    j["Backtrack"] = backtrackConfig;

    // ANTI AIM TODO

    //j["Anti Aim"] = antiAimConfig;

    // GLOW

    j["Glow"] = glowConfig;

    // CHAMS

    j["Chams"] = chamsConfig;
    to_json(j["Chams"]["Toggle Key"], chamsToggleKey, {});
    to_json(j["Chams"]["Hold Key"], chamsHoldKey, {});

    // ESP

    j["ESP"] = espConfig;

    // VISUALS

    j["Visuals"] = visualsConfig;

    // INVENTORY CHANGER

    j["Inventory Changer"] = InventoryChanger::toJson();

    // MISC

    j["Misc"] = miscConfig;

    // STLYE

    j["Style"] = styleConfig;

    removeEmptyObjects(j);

    createConfigDir();
    if (std::ofstream out{ path / configs[id] }; out.good())
        out << std::setw(2) << j;
}

void Config::load(size_t id, bool incremental) noexcept {
    load(configs[id].c_str(), incremental);
}

void Config::load(const char8_t* name, bool incremental) noexcept {
    json j;

    if (std::ifstream in{ path / name }; in.good()) {
        j = json::parse(in, nullptr, false, true);
        if (j.is_discarded())
            return;
    }
    else {
        return;
    }

    if (!incremental)
        reset();

    // AIMBOT

    read(j, "Aimbot", aimbotConfig);
    read(j, "Aimbot On Key", aimbotOnKey);
    read(j, "Aimbot Key", aimbotKey);
    read(j, "Aimbot Key Mode", aimbotKeyMode);

    // TRIGGERBOT

    read(j, "Triggerbot", triggerbotConfig);
    read(j, "Triggerbot Key", triggerbotHoldKey);

    // BACKTRACK

    read<value_t::object>(j, "Backtrack", backtrackConfig);

    // ANTIAIM TODO

    //read<value_t::object>(j, "Anti Aim", antiAimConfig);

    // GLOW

    read(j, "Glow", glowConfig);

    // CHAMS

    read(j, "Chams", chamsConfig);
    read(j["Chams"], "Toggle Key", chamsToggleKey);
    read(j["Chams"], "Hold Key", chamsHoldKey);

    // ESP

    read<value_t::object>(j, "ESP", espConfig);

    // VISUALS

    read<value_t::object>(j, "Visuals", visualsConfig);

    // INVENTORY CHANGER

    InventoryChanger::fromJson(j);

    // MISC

    read<value_t::object>(j, "Misc", miscConfig);

    // STYLE

    read<value_t::object>(j, "Style", styleConfig);
}

void Config::add(const char8_t* name) noexcept {
    if (*name && std::ranges::find(configs, name) == configs.cend()) {
        configs.emplace_back(name);
        save(configs.size() - 1);
    }
}

void Config::remove(size_t id) noexcept {
    std::error_code ec;
    std::filesystem::remove(path / configs[id], ec);
    configs.erase(configs.cbegin() + id);
}

void Config::rename(size_t item, std::u8string_view newName) noexcept {
    std::error_code ec;
    std::filesystem::rename(path / configs[item], path / newName, ec);
    configs[item] = newName;
}

void Config::reset() noexcept {
    aimbotConfig = {};
    triggerbotConfig = {};
    backtrackConfig = {};
    //antiAimConfig = {}; TODO
    glowConfig = {};
    chamsConfig = {};
    espConfig = {};
    visualsConfig = {};
    InventoryChanger::resetConfig();
    miscConfig = {};
    styleConfig = {};
}

void Config::listConfigs() noexcept {
    configs.clear();

    std::error_code ec;
    std::transform(std::filesystem::directory_iterator{ path, ec },
                   std::filesystem::directory_iterator{},
                   std::back_inserter(configs),
                   [](const auto& entry) { return entry.path().filename().u8string(); });
}

void Config::createConfigDir() const noexcept {
    std::error_code ec; std::filesystem::create_directory(path, ec);
}

void Config::openConfigDir() const noexcept {
    createConfigDir();
#ifdef _WIN32
    ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
    int ret = std::system(("xdg-open " + path.string()).c_str());
#endif
}

void Config::scheduleFontLoad(const std::string& name) noexcept {
    scheduledFonts.push_back(name);
}

#ifdef _WIN32
static auto getFontData(const std::string& fontName) noexcept {
    HFONT font = CreateFontA(0, 0, 0, 0,
                             FW_NORMAL, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                             DEFAULT_PITCH, fontName.c_str());

    std::unique_ptr<std::byte[]> data;
    DWORD dataSize = GDI_ERROR;

    if (font) {
        HDC hdc = CreateCompatibleDC(nullptr);

        if (hdc) {
            SelectObject(hdc, font);
            dataSize = GetFontData(hdc, 0, 0, nullptr, 0);

            if (dataSize != GDI_ERROR) {
                data = std::make_unique<std::byte[]>(dataSize);
                dataSize = GetFontData(hdc, 0, 0, data.get(), dataSize);

                if (dataSize == GDI_ERROR)
                    data.reset();
            }
            DeleteDC(hdc);
        }
        DeleteObject(font);
    }
    return std::make_pair(std::move(data), dataSize);
}
#endif

bool Config::loadScheduledFonts() noexcept {
    bool result = false;

    for (const auto& fontName : scheduledFonts) {
        if (fontName == "Default") {
            if (fonts.find("Default") == fonts.cend()) {
                ImFontConfig cfg;
                cfg.OversampleH = cfg.OversampleV = 1;
                cfg.PixelSnapH = true;
                cfg.RasterizerMultiply = 1.7f;

                Font newFont;

                cfg.SizePixels = 13.0f;
                newFont.big = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

                cfg.SizePixels = 10.0f;
                newFont.medium = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

                cfg.SizePixels = 8.0f;
                newFont.tiny = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

                fonts.emplace(fontName, newFont);
                result = true;
            }
            continue;
        }

#ifdef _WIN32
        const auto [fontData, fontDataSize] = getFontData(fontName);
        if (fontDataSize == GDI_ERROR)
            continue;

        if (fonts.find(fontName) == fonts.cend()) {
            const auto ranges = Helpers::getFontGlyphRanges();
            ImFontConfig cfg;
            cfg.FontDataOwnedByAtlas = false;
            cfg.RasterizerMultiply = 1.7f;

            Font newFont;
            newFont.tiny = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 8.0f, &cfg, ranges);
            newFont.medium = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 10.0f, &cfg, ranges);
            newFont.big = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 13.0f, &cfg, ranges);
            fonts.emplace(fontName, newFont);
            result = true;
        }
#endif
    }
    scheduledFonts.clear();
    return result;
}
