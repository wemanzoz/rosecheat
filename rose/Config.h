#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "ConfigStructs.h"
#include "InputUtil.h"

struct ImFont;

class Config {
public:
    Config() noexcept;
    void load(std::size_t, bool incremental) noexcept;
    void load(const char8_t* name, bool incremental) noexcept;
    void save(std::size_t) const noexcept;
    void add(const char8_t*) noexcept;
    void remove(std::size_t) noexcept;
    void rename(std::size_t, std::u8string_view newName) noexcept;
    void reset() noexcept;
    void listConfigs() noexcept;
    void createConfigDir() const noexcept;
    void openConfigDir() const noexcept;

    constexpr auto& getConfigs() noexcept {
        return configs;
    }

    // AIMBOT

    struct Aimbot {
        bool enabled{ false };
        bool aimlock{ false };
        bool silent{ false };
        bool friendlyFire{ false };
        bool visibleOnly{ true };
        bool scopedOnly{ true };
        bool ignoreFlash{ false };
        bool ignoreSmoke{ false };
        bool autoShot{ false };
        bool autoScope{ false };
        float fov{ 0.0f };
        float smooth{ 1.0f };
        int bone{ 0 };
        float maxAimInaccuracy{ 1.0f };
        float maxShotInaccuracy{ 1.0f };
        int minDamage{ 1 };
        bool killshot{ false };
        bool betweenShots{ true };
    };
    std::array<Aimbot, 40> aimbotConfig;
    bool aimbotOnKey{ false };
    KeyBind aimbotKey;
    int aimbotKeyMode{ 0 };

    // TRIGGERBOT

    struct Triggerbot {
        bool enabled = false;
        bool friendlyFire = false;
        bool scopedOnly = true;
        bool ignoreFlash = false;
        bool ignoreSmoke = false;
        bool killshot = false;
        int hitgroup = 0;
        int shotDelay = 0;
        int minDamage = 1;
        float burstTime = 0.0f;
    };
    std::array<Triggerbot, 40> triggerbotConfig;
    KeyBind triggerbotHoldKey;

    // BACKTRACK

    struct BacktrackConfig {
        bool enabled = false;
        bool ignoreSmoke = false;
        bool recoilBasedFov = false;
        int timeLimit = 200;
    } backtrackConfig;

    // ANTI AIM TODO

    /*struct AntiAimConfig {
        bool enabled = false;
    } antiAimConfig;*/

    // GLOW

    struct GlowItem : Color4 {
        bool enabled = false;
        bool healthBased = false;
        int style = 0;
    };

    struct PlayerGlow {
        GlowItem all, visible, occluded;
    };

    std::unordered_map<std::string, PlayerGlow> playerGlowConfig;
    std::unordered_map<std::string, GlowItem> glowConfig;
    KeyBindToggle glowToggleKey;
    KeyBind glowHoldKey;

    // CHAMS

    struct Chams {
        struct Material : Color4 {
            bool enabled = false;
            bool healthBased = false;
            bool blinking = false;
            bool wireframe = false;
            bool cover = false;
            bool ignorez = false;
            int material = 0;
        };
        std::array<Material, 7> materials;
    };

    std::unordered_map<std::string, Chams> chamsConfig;
    KeyBindToggle chamsToggleKey;
    KeyBind chamsHoldKey;

    // ESP

    struct ESP {
        KeyBindToggle toggleKey;
        KeyBind holdKey;

        std::unordered_map<std::string, Player> allies;
        std::unordered_map<std::string, Player> enemies;
        std::unordered_map<std::string, Weapon> weapons;
        std::unordered_map<std::string, Projectile> projectiles;
        std::unordered_map<std::string, Shared> lootCrates;
        std::unordered_map<std::string, Shared> otherEntities;
    } espConfig;

    // VISUALS

    struct BulletTracers : ColorToggle {
        BulletTracers() : ColorToggle{ 0.0f, 0.75f, 1.0f, 1.0f } {}
    };

    struct VisualsConfig {
        bool disablePostProcessing{ false };
        bool inverseRagdollGravity{ false };
        bool noFog{ false };
        bool no3dSky{ false };
        bool noAimPunch{ false };
        bool noViewPunch{ false };
        bool noHands{ false };
        bool noSleeves{ false };
        bool noWeapons{ false };
        bool noSmoke{ false };
        bool noBlur{ false };
        bool noScopeOverlay{ false };
        bool noGrass{ false };
        bool noShadows{ false };
        bool wireframeSmoke{ false };
        bool zoom{ false };
        KeyBindToggle zoomKey;
        bool thirdperson{ false };
        KeyBindToggle thirdpersonKey;
        int thirdpersonDistance{ 0 };
        int viewmodelFov{ 0 };
        int fov{ 0 };
        int farZ{ 0 };
        int flashReduction{ 0 };
        float brightness{ 0.0f };
        int skybox{ 0 };
        ColorToggle3 world;
        ColorToggle3 sky;
        bool deagleSpinner{ false };
        int screenEffect{ 0 };
        int hitEffect{ 0 };
        float hitEffectTime{ 0.6f };
        int hitMarker{ 0 };
        float hitMarkerTime{ 0.6f };
        float maxAngleDelta{ 255.0f };
        BulletTracers bulletTracers;
        ColorToggle molotovHull{ 1.0f, 0.27f, 0.0f, 0.3f };

        struct ColorCorrection {
            bool enabled = false;
            float blue = 0.0f;
            float red = 0.0f;
            float mono = 0.0f;
            float saturation = 0.0f;
            float ghost = 0.0f;
            float green = 0.0f;
            float yellow = 0.0f;
        } colorCorrection;
    } visualsConfig;

    // MISC
    struct MiscConfig {
        MiscConfig() { clanTag[0] = '\0'; }

        KeyBind menuKey{ KeyBind::INSERT };
        bool antiAfk{ false };
        bool bunnyHop{ false };
        bool customClanTag{ false };
        bool animatedClanTag{ false };
        bool fastDuck{ false };
        bool moonWalk{ false };
        bool edgeJump{ false };
        bool slowWalk{ false };
        bool autoAccept{ false };
        bool infiniteRadar{ false };
        bool revealRanks{ false };
        bool revealMoney{ false };
        bool revealSuspect{ false };
        bool revealVotes{ false };
        bool fixAnimationLOD{ false };
        bool fixBoneMatrix{ false };
        bool fixMovement{ false };
        bool disableModelOcclusion{ false };
        bool nameStealer{ false };
        bool killMessage{ false };
        bool nadePredict{ false };
        bool infiniteTablet{ false };
        bool fastStop{ false };
        bool prepareRevolver{ false };
        float maxAngleDelta{ 255.0f };
        char clanTag[16];
        KeyBind edgeJumpKey;
        KeyBind slowWalkKey;
        ColorToggleThickness noscopeCrosshair;
        ColorToggleThickness recoilCrosshair;
        bool customPlayerName{ false };
        std::string playerName{ "lemon cake the best" };
        std::string killMessageString{ "vacban.wtf on top" };
        ColorToggle3 bombTimer{ 1.0f, 0.55f, 0.0f };
        int hitSound{ 0 };
        int killSound{ 0 };
        std::string customKillSound;
        std::string customHitSound;

        struct Reportbot {
            bool enabled = false;
            bool textAbuse = false;
            bool griefing = false;
            bool wallhack = true;
            bool aimbot = true;
            bool other = true;
            int target = 0;
            int delay = 1;
            int rounds = 1;
        } reportbot;

        struct SpectatorList {
            bool enabled = false;
            bool noTitleBar = false;
            ImVec2 pos;
            ImVec2 size{ 200.0f, 200.0f };
        }; SpectatorList spectatorList;

        struct PurchaseList {
            bool enabled = false;
            bool onlyDuringFreezeTime = false;
            bool showPrices = false;
            bool noTitleBar = false;

            enum Mode {
                Details = 0,
                Summary
            };
            int mode = Details;
        }; PurchaseList purchaseList;

        struct TeamDamageList {
            bool enabled = false;
            bool noTitleBar = false;
            ImVec2 pos;
            ImVec2 size{ 400.0f, 400.0f };
        }; TeamDamageList teamDamageList;

        struct PreserveKillfeed {
            bool enabled = false;
            bool onlyHeadshots = false;
        }; PreserveKillfeed preserveKillfeed;

        struct OffscreenEnemies : ColorToggle {
            OffscreenEnemies() : ColorToggle{ 1.0f, 0.26f, 0.21f, 1.0f } {}
            HealthBar healthBar;
        }; OffscreenEnemies offscreenEnemies;

        struct Watermark {
            bool enabled = false;
        }; Watermark watermark;
    } miscConfig;

    // STYLE

    struct Style {
        int menuStyle{ 0 };
        int menuColours{ 0 };
        ColorToggle3 menuColour{ 0.6f, 0.0f, 0.6f };
    } styleConfig;

    struct Font {
        ImFont* tiny;
        ImFont* medium;
        ImFont* big;
    };

    void scheduleFontLoad(const std::string& name) noexcept;
    bool loadScheduledFonts() noexcept;
    const auto& getSystemFonts() const noexcept { return systemFonts; }
    const auto& getFonts() const noexcept { return fonts; }
private:
    std::vector<std::string> scheduledFonts{ "Default" };
    std::vector<std::string> systemFonts{ "Default" };
    std::unordered_map<std::string, Font> fonts;
    std::filesystem::path path;
    std::vector<std::u8string> configs;
};

inline std::unique_ptr<Config> config;
