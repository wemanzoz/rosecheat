/* TODO
#include <cmath>

#include "AntiAim.h"

#include "../imgui/imgui.h"

#include "../ConfigStructs.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"
#include "../Config.h"
#include "../Hooks.h"

// https://github.com/CSGOLeaks/Legendware-V3/blob/main/cheats/ragebot/antiaim.cpp
// https://github.com/CSGOLeaks/Rifk7/blob/master/hacks/c_antiaim.cpp

#if OSIRIS_ANTIAIM()

void AntiAim::run(LocalPlayer* localPlayer, UserCmd* cmd) {
    static constexpr auto targetDelta = -60.0f;
    static auto alternate = false;

    const auto weapon = EntityList::getEntityFromHandle(localPlayer->get()->handle());
}

void AntiAim::prepare_animation(LocalPlayer* localPlayer) {}

void AntiAim::fakelag(LocalPlayer* localPlayer, UserCmd* cmd, bool& send_packet) {}

void AntiAim::predict(LocalPlayer* localPlayer, UserCmd* cmd) {}

float AntiAim::getVisualChoke() {
    return 0.0f;
}

void AntiAim::incrementVisualProgress() {}

float AntiAim::getLastReal() {
    return 0.0f;
}

float AntiAim::getLastFake() {
    return 0.0f;
}

bool AntiAim::onPeek(LocalPlayer* localPlayer, bool& target) {
    return false;
}

float AntiAim::calculateIdealYaw(LocalPlayer* localPlayer, bool estimate) {
    return 0.0f;
}

#else

class AntiAim {
public:
    void run(LocalPlayer* localPlayer, UserCmd* cmd);
    void prepare_animation(LocalPlayer* localPlayer);
    void fakelag(LocalPlayer* localPlayer, UserCmd* cmd, bool& send_packet);
    void predict(LocalPlayer* localPlayer, UserCmd* cmd);
    float getVisualChoke();
    void incrementVisualProgress();
    float getLastReal();
    float getLastFake();

    uint32_t shot_cmd{};
private:
    bool onPeek(LocalPlayer* localPlayer, bool& target);
    float calculateIdealYaw(LocalPlayer* localPlayer, bool estimate = false);

    float visualChoke = 0.0f,
        lastReal = 0.0f, lastFake = 0.0f,
        nextLbyUpdate = 0.0f, lbyUpdate = 0.0f,
        stopToRunningFraction = 0.0f,
        feetStandingSpeed = 0.0f,
        feetDuckSpeed = 0.0f;
    bool isStanding = false;
    uint32_t estimatedChoke = 0;
};

#endif
*/
