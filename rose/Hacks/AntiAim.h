/* TODO
#pragma once
#include "../JsonForward.h"
#include "../SDK/LocalPlayer.h"

struct UserCmd;
struct Vector;

#define OSIRIS_ANTIAIM() true

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
*/
