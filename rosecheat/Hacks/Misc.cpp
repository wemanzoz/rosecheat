#include <algorithm>
#include <array>
#include <iomanip>
#include <mutex>
#include <numbers>
#include <numeric>
#include <sstream>
#include <vector>

#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"
#include "../imgui/imgui_stdlib.h"

#include "../ConfigStructs.h"
#include "../InputUtil.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../ProtobufReader.h"

#include "EnginePrediction.h"
#include "Misc.h"

#include "../SDK/ClassId.h"
#include "../SDK/Client.h"
#include "../SDK/ClientClass.h"
#include "../SDK/ClientMode.h"
#include "../SDK/ConVar.h"
#include "../SDK/Cvar.h"
#include "../SDK/Engine.h"
#include "../SDK/EngineTrace.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GameEvent.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/ItemSchema.h"
#include "../SDK/Localize.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/Panorama.h"
#include "../SDK/Platform.h"
#include "../SDK/UserCmd.h"
#include "../SDK/UtlVector.h"
#include "../SDK/Vector.h"
#include "../SDK/WeaponData.h"
#include "../SDK/WeaponId.h"
#include "../SDK/WeaponSystem.h"

#include "../GUI.h"
#include "../Helpers.h"
#include "../Hooks.h"
#include "../GameData.h"

#include "../imguiCustom.h"

#include "../Config.h"

float Misc::maxAngleDelta() noexcept {
    return config->miscConfig.maxAngleDelta;
}

bool Misc::shouldRevealMoney() noexcept {
    return config->miscConfig.revealMoney;
}

bool Misc::shouldRevealSuspect() noexcept {
    return config->miscConfig.revealSuspect;
}

bool Misc::shouldDisableModelOcclusion() noexcept {
    return config->miscConfig.disableModelOcclusion;
}

bool Misc::shouldFixBoneMatrix() noexcept {
    return config->miscConfig.fixBoneMatrix;
}

bool Misc::isRadarHackOn() noexcept {
    return config->miscConfig.infiniteRadar;
}

bool Misc::isMenuKeyPressed() noexcept {
    return config->miscConfig.menuKey.isPressed();
}

void Misc::edgeJump(UserCmd* cmd) noexcept {
    if (!config->miscConfig.edgeJump || !config->miscConfig.edgeJumpKey.isDown())
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto mt = localPlayer->moveType(); mt == MoveType::LADDER || mt == MoveType::NOCLIP)
        return;

    if ((EnginePrediction::getFlags() & 1) && !(localPlayer->flags() & 1))
        cmd->buttons |= UserCmd::IN_JUMP;
}

void Misc::slowWalk(UserCmd* cmd) noexcept {
    if (!config->miscConfig.slowWalk || !config->miscConfig.slowWalkKey.isDown())
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    const auto weaponData = activeWeapon->getWeaponData();
    if (!weaponData)
        return;

    const float maxSpeed = (localPlayer->isScoped() ? weaponData->maxSpeedAlt : weaponData->maxSpeed) / 3;

    if (cmd->forwardmove && cmd->sidemove) {
        const float maxSpeedRoot = maxSpeed * static_cast<float>(M_SQRT1_2);
        cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
        cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
    }
    else if (cmd->forwardmove) {
        cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeed : maxSpeed;
    }
    else if (cmd->sidemove) {
        cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeed : maxSpeed;
    }
}

void Misc::updateClanTag(bool tagChanged) noexcept {
    static std::string clanTag;

    if (tagChanged) {
        clanTag = config->miscConfig.clanTag;
        if (!clanTag.empty() && clanTag.front() != ' ' && clanTag.back() != ' ')
            clanTag.push_back(' ');
        return;
    }

    static auto lastTime = 0.0f;

    if (config->miscConfig.customClanTag) {
        if (memory->globalVars->realtime - lastTime < 0.6f)
            return;

        if (config->miscConfig.animatedClanTag && !clanTag.empty()) {
            if (const auto offset = Helpers::utf8SeqLen(clanTag[0]); offset <= clanTag.length())
                std::rotate(clanTag.begin(), clanTag.begin() + offset, clanTag.end());
        }
        lastTime = memory->globalVars->realtime;
        memory->setClanTag(clanTag.c_str(), clanTag.c_str());
    }
}

void Misc::updatePlayerName() noexcept {
    if (config->miscConfig.playerName == "" || !config->miscConfig.customPlayerName)
        return;

    changeName(false, config->miscConfig.playerName.c_str(), 0.1f);
}

void Misc::spectatorList() noexcept {
    if (!config->miscConfig.spectatorList.enabled)
        return;

    GameData::Lock lock;

    const auto& observers = GameData::observers();

    if (std::ranges::none_of(observers, [](const auto& obs) { return obs.targetIsLocalPlayer; }) && !gui->isOpen())
        return;

    if (config->miscConfig.spectatorList.pos != ImVec2{}) {
        ImGui::SetNextWindowPos(config->miscConfig.spectatorList.pos);
        config->miscConfig.spectatorList.pos = {};
    }

    if (config->miscConfig.spectatorList.size != ImVec2{}) {
        ImGui::SetNextWindowSize(ImClamp(config->miscConfig.spectatorList.size, {}, ImGui::GetIO().DisplaySize));
        config->miscConfig.spectatorList.size = {};
    }

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
    if (!gui->isOpen())
        windowFlags |= ImGuiWindowFlags_NoInputs;
    if (config->miscConfig.spectatorList.noTitleBar)
        windowFlags |= ImGuiWindowFlags_NoTitleBar;

    if (!gui->isOpen())
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::GetColorU32(ImGuiCol_TitleBgActive));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
    ImGui::Begin("Spectator List", nullptr, windowFlags);
    ImGui::PopStyleVar();

    if (!gui->isOpen())
        ImGui::PopStyleColor();

    for (const auto& observer : observers) {
        if (!observer.targetIsLocalPlayer)
            continue;

        if (const auto it = std::ranges::find(GameData::players(), observer.playerHandle, &PlayerData::handle); it != GameData::players().cend()) {
            if (const auto texture = it->getAvatarTexture()) {
                const auto textSize = ImGui::CalcTextSize(it->name.c_str());
                ImGui::Image(texture, ImVec2(textSize.y, textSize.y), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.3f));
                ImGui::SameLine();
                ImGui::TextWrapped("%s", it->name.c_str());
            }
        }
    }

    ImGui::End();
}

void Misc::purchaseList(GameEvent* event) noexcept {
    static std::mutex mtx;
    std::scoped_lock _{ mtx };

    struct PlayerPurchases {
        int totalCost;
        std::unordered_map<std::string, int> items;
    };

    static std::unordered_map<int, PlayerPurchases> playerPurchases;
    static std::unordered_map<std::string, int> purchaseTotal;
    static int totalCost;

    static auto freezeEnd = 0.0f;

    if (event) {
        switch (fnv::hashRuntime(event->getName())) {
            case fnv::hash("item_purchase"):
            {
                if (const auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid"))); player && localPlayer && localPlayer->isOtherEnemy(player)) {
                    if (const auto definition = memory->itemSystem()->getItemSchema()->getItemDefinitionByName(event->getString("weapon"))) {
                        auto& purchase = playerPurchases[player->handle()];
                        if (const auto weaponInfo = memory->weaponSystem->getWeaponInfo(definition->getWeaponId())) {
                            purchase.totalCost += weaponInfo->price;
                            totalCost += weaponInfo->price;
                        }
                        const std::string weapon = interfaces->localize->findAsUTF8(definition->getItemBaseName());
                        ++purchaseTotal[weapon];
                        ++purchase.items[weapon];
                    }
                }
                break;
            }
            case fnv::hash("round_start"):
                freezeEnd = 0.0f;
                playerPurchases.clear();
                purchaseTotal.clear();
                totalCost = 0;
                break;
            case fnv::hash("round_freeze_end"):
                freezeEnd = memory->globalVars->realtime;
                break;
        }
    }
    else {
        if (!config->miscConfig.purchaseList.enabled)
            return;

        if (static const auto mp_buytime = interfaces->cvar->findVar("mp_buytime"); (!interfaces->engine->isInGame() || freezeEnd != 0.0f && memory->globalVars->realtime > freezeEnd + (!config->miscConfig.purchaseList.onlyDuringFreezeTime ? mp_buytime->getFloat() : 0.0f) || playerPurchases.empty() || purchaseTotal.empty()) && !gui->isOpen())
            return;

        ImGui::SetNextWindowSize({ 200.0f, 200.0f }, ImGuiCond_Once);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
        if (!gui->isOpen())
            windowFlags |= ImGuiWindowFlags_NoInputs;
        if (config->miscConfig.purchaseList.noTitleBar)
            windowFlags |= ImGuiWindowFlags_NoTitleBar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
        ImGui::Begin("Purchases", nullptr, windowFlags);
        ImGui::PopStyleVar();

        if (config->miscConfig.purchaseList.mode == Config::MiscConfig::PurchaseList::Details) {
            GameData::Lock lock;

            for (const auto& [handle, purchases] : playerPurchases) {
                std::string s;
                s.reserve(std::accumulate(purchases.items.begin(), purchases.items.end(), 0, [](int length, const auto& p) { return length + p.first.length() + 2; }));
                for (const auto& purchasedItem : purchases.items) {
                    if (purchasedItem.second > 1)
                        s += std::to_string(purchasedItem.second) + "x ";
                    s += purchasedItem.first + ", ";
                }

                if (s.length() >= 2)
                    s.erase(s.length() - 2);

                if (const auto player = GameData::playerByHandle(handle)) {
                    if (config->miscConfig.purchaseList.showPrices)
                        ImGui::TextWrapped("%s $%d: %s", player->name.c_str(), purchases.totalCost, s.c_str());
                    else
                        ImGui::TextWrapped("%s: %s", player->name.c_str(), s.c_str());
                }
            }
        }
        else if (config->miscConfig.purchaseList.mode == Config::MiscConfig::PurchaseList::Summary) {
            for (const auto& purchase : purchaseTotal)
                ImGui::TextWrapped("%d x %s", purchase.second, purchase.first.c_str());

            if (config->miscConfig.purchaseList.showPrices && totalCost > 0) {
                ImGui::Separator();
                ImGui::TextWrapped("Total: $%d", totalCost);
            }
        }
        ImGui::End();
    }
}

void Misc::teamDamageList(GameEvent* event) noexcept {
    if (!config->miscConfig.teamDamageList.enabled)
        return;

    static std::mutex mtx;
    std::scoped_lock _{ mtx };

    struct PlayerDamage {
        int damage;
    }; static std::unordered_map<int, PlayerDamage> playerDamage;

    // If the event exists
    if (event) {
        // If the event is player_hurt
        if (fnv::hashRuntime("player_hurt")) {
            // Getting the attacker, checking if it exists, if you exist, etc
            if (const auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("attacker"))); player && localPlayer && !localPlayer->isOtherEnemy(player)) {
                // Add the damage dealt to the team member
                playerDamage[player->handle()].damage += (event->getInt("dmg_health") + event->getInt("dmg_armor"));
            }

            // If the position is non-existent or invalid
            if (config->miscConfig.teamDamageList.pos != ImVec2{}) {
                ImGui::SetNextWindowPos(config->miscConfig.teamDamageList.pos);
                config->miscConfig.teamDamageList.pos = {};
            }

            // If the size is non-existent or invalid
            if (config->miscConfig.teamDamageList.size != ImVec2{}) {
                ImGui::SetNextWindowSize(ImClamp(config->miscConfig.teamDamageList.size, {}, ImGui::GetIO().DisplaySize));
                config->miscConfig.teamDamageList.size = {};
            }

            // Setting window stuff
            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
            if (!gui->isOpen())
                windowFlags |= ImGuiWindowFlags_NoInputs;
            if (config->miscConfig.spectatorList.noTitleBar)
                windowFlags |= ImGuiWindowFlags_NoTitleBar;

            if (!gui->isOpen())
                ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::GetColorU32(ImGuiCol_TitleBgActive));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
            ImGui::Begin("Team Damage List", nullptr, windowFlags);
            ImGui::PopStyleVar();

            if (!gui->isOpen())
                ImGui::PopStyleColor();

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            for (const auto& player : GameData::players()) {
                if (!localPlayer->isOtherEnemy(interfaces->entityList->getEntityFromHandle(localPlayer->handle())))
                    continue;

                if (const auto it = std::ranges::find(GameData::players(), player.handle, &PlayerData::handle); it != GameData::players().cend()) {
                    if (const auto texture = it->getAvatarTexture()) {
                        const auto textSize = ImGui::CalcTextSize(it->name.c_str());
                        ImGui::Image(texture, ImVec2(textSize.y, textSize.y), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.3f));
                        ImGui::SameLine();
                        ImGui::TextWrapped("%s", it->name.c_str());
                        //drawList->AddRectFilled(ImGui::GetItemRectSize());
                    }
                }
            }
        }
        ImGui::End();
    }
}

static void drawCrosshair(ImDrawList* drawList, const ImVec2& pos, ImU32 color) noexcept {
    // dot
    drawList->AddRectFilled(pos - ImVec2{ 1, 1 }, pos + ImVec2{ 2, 2 }, color & IM_COL32_A_MASK);
    drawList->AddRectFilled(pos, pos + ImVec2{ 1, 1 }, color);

    // left
    drawList->AddRectFilled(ImVec2{ pos.x - 11, pos.y - 1 }, ImVec2{ pos.x - 3, pos.y + 2 }, color & IM_COL32_A_MASK);
    drawList->AddRectFilled(ImVec2{ pos.x - 10, pos.y }, ImVec2{ pos.x - 4, pos.y + 1 }, color);

    // right
    drawList->AddRectFilled(ImVec2{ pos.x + 4, pos.y - 1 }, ImVec2{ pos.x + 12, pos.y + 2 }, color & IM_COL32_A_MASK);
    drawList->AddRectFilled(ImVec2{ pos.x + 5, pos.y }, ImVec2{ pos.x + 11, pos.y + 1 }, color);

    // top (left with swapped x/y offsets)
    drawList->AddRectFilled(ImVec2{ pos.x - 1, pos.y - 11 }, ImVec2{ pos.x + 2, pos.y - 3 }, color & IM_COL32_A_MASK);
    drawList->AddRectFilled(ImVec2{ pos.x, pos.y - 10 }, ImVec2{ pos.x + 1, pos.y - 4 }, color);

    // bottom (right with swapped x/y offsets)
    drawList->AddRectFilled(ImVec2{ pos.x - 1, pos.y + 4 }, ImVec2{ pos.x + 2, pos.y + 12 }, color & IM_COL32_A_MASK);
    drawList->AddRectFilled(ImVec2{ pos.x, pos.y + 5 }, ImVec2{ pos.x + 1, pos.y + 11 }, color);
}

void Misc::noscopeCrosshair(ImDrawList* drawList) noexcept {
    if (!config->miscConfig.noscopeCrosshair.asColorToggle().enabled)
        return;

    {
        GameData::Lock lock;
        if (const auto& local = GameData::local(); !local.exists || !local.alive || !local.noScope)
            return;
    }

    drawCrosshair(drawList, ImGui::GetIO().DisplaySize / 2, Helpers::calculateColor(config->miscConfig.noscopeCrosshair.asColorToggle().asColor4()));
}

void Misc::recoilCrosshair(ImDrawList* drawList) noexcept {
    if (!config->miscConfig.recoilCrosshair.asColorToggle().enabled)
        return;

    GameData::Lock lock;
    const auto& localPlayerData = GameData::local();

    if (!localPlayerData.exists || !localPlayerData.alive)
        return;

    if (!localPlayerData.shooting)
        return;

    if (ImVec2 pos; Helpers::worldToScreenPixelAligned(localPlayerData.aimPunch, pos))
        drawCrosshair(drawList, pos, Helpers::calculateColor(config->miscConfig.recoilCrosshair.asColorToggle().asColor4()));
}

void Misc::watermark() noexcept {
    if (!config->miscConfig.watermark.enabled)
        return;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;
    if (!gui->isOpen())
        windowFlags |= ImGuiWindowFlags_NoInputs;

    ImGui::SetNextWindowBgAlpha(0.3f);
    ImGui::Begin("Watermark", nullptr, windowFlags);

    static auto frameRate = 1.0f;
    frameRate = 0.9f * frameRate + 0.1f * memory->globalVars->absoluteFrameTime;

    ImGui::Text("Rose Cheat | % d fps | % d ms", frameRate != 0.0f ? static_cast<int>(1 / frameRate) : 0, GameData::getNetOutgoingLatency());
    ImGui::End();
}

void Misc::prepareRevolver(UserCmd* cmd) noexcept {
    constexpr auto timeToTicks = [](float time) {  return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); };
    constexpr float revolverPrepareTime{ 0.234375f };

    static float readyTime;
    if (config->miscConfig.prepareRevolver && localPlayer) {
        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (activeWeapon && activeWeapon->itemDefinitionIndex() == WeaponId::Revolver) {
            if (!readyTime) readyTime = memory->globalVars->serverTime() + revolverPrepareTime;
            auto ticksToReady = timeToTicks(readyTime - memory->globalVars->serverTime() - interfaces->engine->getNetworkChannel()->getLatency(0));
            if (ticksToReady > 0 && ticksToReady <= timeToTicks(revolverPrepareTime))
                cmd->buttons |= UserCmd::IN_ATTACK;
            else
                readyTime = 0.0f;
        }
    }
}

void Misc::fastStop(UserCmd* cmd) noexcept {
    if (!config->miscConfig.fastStop)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER || !(localPlayer->flags() & 1) || cmd->buttons & UserCmd::IN_JUMP)
        return;

    if (cmd->buttons & (UserCmd::IN_MOVELEFT | UserCmd::IN_MOVERIGHT | UserCmd::IN_FORWARD | UserCmd::IN_BACK))
        return;

    const auto velocity = localPlayer->velocity();
    const auto speed = velocity.length2D();
    if (speed < 15.0f)
        return;

    Vector direction = velocity.toAngle();
    direction.y = cmd->viewangles.y - direction.y;

    const auto negatedDirection = Vector::fromAngle(direction) * -speed;
    cmd->forwardmove = negatedDirection.x;
    cmd->sidemove = negatedDirection.y;
}

void Misc::drawBombTimer() noexcept {
    if (!config->miscConfig.bombTimer.enabled)
        return;

    GameData::Lock lock;

    const auto& plantedC4 = GameData::plantedC4();
    if (plantedC4.blowTime == 0.0f && !gui->isOpen())
        return;

    if (!gui->isOpen()) {
        ImGui::SetNextWindowBgAlpha(0.3f);
    }

    static float windowWidth = 200.0f;
    ImGui::SetNextWindowPos({ (ImGui::GetIO().DisplaySize.x - 200.0f) / 2.0f, 60.0f }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ windowWidth, 0 }, ImGuiCond_Once);

    if (!gui->isOpen())
        ImGui::SetNextWindowSize({ windowWidth, 0 });

    ImGui::SetNextWindowSizeConstraints({ 0, -1 }, { FLT_MAX, -1 });
    ImGui::Begin("Bomb Timer", nullptr, ImGuiWindowFlags_NoTitleBar | (gui->isOpen() ? 0 : ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration));

    std::ostringstream ss; ss << "Bomb on " << (!plantedC4.bombsite ? 'A' : 'B') << " : " << std::fixed << std::showpoint << std::setprecision(3) << (std::max)(plantedC4.blowTime - memory->globalVars->currenttime, 0.0f) << " s";

    ImGui::textUnformattedCentered(ss.str().c_str());

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Helpers::calculateColor(config->miscConfig.bombTimer.asColor3()));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f });
    ImGui::progressBarFullWidth((plantedC4.blowTime - memory->globalVars->currenttime) / plantedC4.timerLength, 5.0f);

    if (plantedC4.defuserHandle != -1) {
        const bool canDefuse = plantedC4.blowTime >= plantedC4.defuseCountDown;

        if (plantedC4.defuserHandle == GameData::local().handle) {
            if (canDefuse) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
                ImGui::textUnformattedCentered("You can defuse!");
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                ImGui::textUnformattedCentered("You can not defuse!");
            }
            ImGui::PopStyleColor();
        }
        else if (const auto defusingPlayer = GameData::playerByHandle(plantedC4.defuserHandle)) {
            std::ostringstream ss; ss << defusingPlayer->name << " is defusing: " << std::fixed << std::showpoint << std::setprecision(3) << (std::max)(plantedC4.defuseCountDown - memory->globalVars->currenttime, 0.0f) << " s";

            ImGui::textUnformattedCentered(ss.str().c_str());

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, canDefuse ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255));
            ImGui::progressBarFullWidth((plantedC4.defuseCountDown - memory->globalVars->currenttime) / plantedC4.defuseLength, 5.0f);
            ImGui::PopStyleColor();
        }
    }

    windowWidth = ImGui::GetCurrentWindow()->SizeFull.x;

    ImGui::PopStyleColor(2);
    ImGui::End();
}

void Misc::stealNames() noexcept {
    if (!config->miscConfig.nameStealer)
        return;

    if (!localPlayer)
        return;

    static std::vector<int> stolenIds;

    for (int i = 1; i <= memory->globalVars->maxClients; ++i) {
        const auto entity = interfaces->entityList->getEntity(i);

        if (!entity || entity == localPlayer.get())
            continue;

        PlayerInfo playerInfo;
        if (!interfaces->engine->getPlayerInfo(entity->index(), playerInfo))
            continue;

        if (playerInfo.fakeplayer || std::ranges::find(stolenIds, playerInfo.userId) != stolenIds.cend())
            continue;

        if (changeName(false, (std::string{ playerInfo.name } + '\x1').c_str(), 1.0f))
            stolenIds.push_back(playerInfo.userId);

        return;
    }
    stolenIds.clear();
}

bool Misc::changeName(bool reconnect, const char* newName, float delay) noexcept {
    static auto exploitInitialized{ false };

    static auto name{ interfaces->cvar->findVar("name") };

    if (reconnect) {
        exploitInitialized = false;
        return false;
    }

    if (!exploitInitialized && interfaces->engine->isInGame()) {
        if (PlayerInfo playerInfo; localPlayer && interfaces->engine->getPlayerInfo(localPlayer->index(), playerInfo) && (!strcmp(playerInfo.name, "?empty") || !strcmp(playerInfo.name, "\n\xAD\xAD\xAD"))) {
            exploitInitialized = true;
        }
        else {
            name->onChangeCallbacks.size = 0;
            name->setValue("\n\xAD\xAD\xAD");
            return false;
        }
    }

    if (static auto nextChangeTime = 0.0f; nextChangeTime <= memory->globalVars->realtime) {
        name->setValue(newName);
        nextChangeTime = memory->globalVars->realtime + delay;
        return true;
    }
    return false;
}

void Misc::bunnyHop(UserCmd* cmd) noexcept {
    if (!localPlayer)
        return;

    static auto wasLastTimeOnGround{ localPlayer->flags() & 1 };

    if (config->miscConfig.bunnyHop && !(localPlayer->flags() & 1) && localPlayer->moveType() != MoveType::LADDER && !wasLastTimeOnGround)
        cmd->buttons &= ~UserCmd::IN_JUMP;

    wasLastTimeOnGround = localPlayer->flags() & 1;
}

void Misc::nadePredict() noexcept {
    static auto nadeVar{ interfaces->cvar->findVar("cl_grenadepreview") };

    nadeVar->onChangeCallbacks.size = 0;
    nadeVar->setValue(config->miscConfig.nadePredict);
}

void Misc::infiniteTablet() noexcept {
    if (config->miscConfig.infiniteTablet && localPlayer) {
        if (auto activeWeapon{ localPlayer->getActiveWeapon() }; activeWeapon && activeWeapon->getClientClass()->classId == ClassId::Tablet)
            activeWeapon->tabletReceptionIsBlocked() = false;
    }
}

void Misc::killMessage(GameEvent& event) noexcept {
    if (!config->miscConfig.killMessage)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt("attacker") != localUserId || event.getInt("userid") == localUserId)
        return;

    std::string cmd = "say \"";
    cmd += config->miscConfig.killMessageString;
    cmd += '"';
    interfaces->engine->clientCmdUnrestricted(cmd.c_str());
}

void Misc::fixMovement(UserCmd* cmd, float yaw) noexcept {
    if (config->miscConfig.fixMovement) {
        float oldYaw = yaw + (yaw < 0.0f ? 360.0f : 0.0f);
        float newYaw = cmd->viewangles.y + (cmd->viewangles.y < 0.0f ? 360.0f : 0.0f);
        float yawDelta = newYaw < oldYaw ? fabsf(newYaw - oldYaw) : 360.0f - fabsf(newYaw - oldYaw);
        yawDelta = 360.0f - yawDelta;

        const float forwardmove = cmd->forwardmove;
        const float sidemove = cmd->sidemove;
        cmd->forwardmove = std::cos(Helpers::deg2rad(yawDelta)) * forwardmove + std::cos(Helpers::deg2rad(yawDelta + 90.0f)) * sidemove;
        cmd->sidemove = std::sin(Helpers::deg2rad(yawDelta)) * forwardmove + std::sin(Helpers::deg2rad(yawDelta + 90.0f)) * sidemove;
    }
}

void Misc::antiAfk(UserCmd* cmd) noexcept {
    if (config->miscConfig.antiAfk && cmd->commandNumber % 2)
        cmd->buttons |= 1 << 26;
}

void Misc::fixAnimationLOD(FrameStage stage) noexcept {
#ifdef _WIN32
    if (config->miscConfig.fixAnimationLOD && stage == FrameStage::RENDER_START) {
        if (!localPlayer)
            return;

        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
            Entity* entity = interfaces->entityList->getEntity(i);
            if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()) continue;
            *reinterpret_cast<int*>(entity + 0xA28) = 0;
            *reinterpret_cast<int*>(entity + 0xA30) = memory->globalVars->framecount;
        }
    }
#endif
}

void Misc::revealRanks(UserCmd* cmd) noexcept {
    if (config->miscConfig.revealRanks && cmd->buttons & UserCmd::IN_SCORE)
        interfaces->client->dispatchUserMessage(50, 0, 0, nullptr);
}

void Misc::removeCrouchCooldown(UserCmd* cmd) noexcept {
    if (config->miscConfig.fastDuck)
        cmd->buttons |= UserCmd::IN_BULLRUSH;
}

void Misc::moonWalk(UserCmd* cmd) noexcept {
    if (config->miscConfig.moonWalk && localPlayer && localPlayer->moveType() != MoveType::LADDER)
        cmd->buttons ^= UserCmd::IN_FORWARD | UserCmd::IN_BACK | UserCmd::IN_MOVELEFT | UserCmd::IN_MOVERIGHT;
}

void Misc::playHitSound(GameEvent& event) noexcept {
    if (!config->miscConfig.hitSound)
        return;

    if (!localPlayer)
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt("attacker") != localUserId || event.getInt("userid") == localUserId)
        return;

    constexpr std::array hitSounds{
        "play physics/metal/metal_solid_impact_bullet2",
        "play buttons/arena_switch_press_02",
        "play training/timer_bell",
        "play physics/glass/glass_impact_bullet1"
    };

    if (static_cast<std::size_t>(config->miscConfig.hitSound - 1) < hitSounds.size())
        interfaces->engine->clientCmdUnrestricted(hitSounds[config->miscConfig.hitSound - 1]);
    else if (config->miscConfig.hitSound == 5)
        interfaces->engine->clientCmdUnrestricted(("play " + config->miscConfig.customHitSound).c_str());
}

void Misc::killSound(GameEvent& event) noexcept {
    if (!config->miscConfig.killSound)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt("attacker") != localUserId || event.getInt("userid") == localUserId)
        return;

    constexpr std::array killSounds{
        "play physics/metal/metal_solid_impact_bullet2",
        "play buttons/arena_switch_press_02",
        "play training/timer_bell",
        "play physics/glass/glass_impact_bullet1"
    };

    if (static_cast<std::size_t>(config->miscConfig.killSound - 1) < killSounds.size())
        interfaces->engine->clientCmdUnrestricted(killSounds[config->miscConfig.killSound - 1]);
    else if (config->miscConfig.killSound == 5)
        interfaces->engine->clientCmdUnrestricted(("play " + config->miscConfig.customKillSound).c_str());
}

static std::vector<std::uint64_t> reportedPlayers;
static int reportbotRound;

void Misc::runReportbot() noexcept {
    if (!config->miscConfig.reportbot.enabled)
        return;

    if (!localPlayer)
        return;

    static auto lastReportTime = 0.0f;

    if (lastReportTime + config->miscConfig.reportbot.delay > memory->globalVars->realtime)
        return;

    if (reportbotRound >= config->miscConfig.reportbot.rounds)
        return;

    for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
        const auto entity = interfaces->entityList->getEntity(i);

        if (!entity || entity == localPlayer.get())
            continue;

        if (config->miscConfig.reportbot.target != 2 && (localPlayer->isOtherEnemy(entity) ? config->miscConfig.reportbot.target != 0 : config->miscConfig.reportbot.target != 1))
            continue;

        PlayerInfo playerInfo;
        if (!interfaces->engine->getPlayerInfo(i, playerInfo))
            continue;

        if (playerInfo.fakeplayer || std::ranges::find(reportedPlayers, playerInfo.xuid) != reportedPlayers.cend())
            continue;

        std::string report;

        if (config->miscConfig.reportbot.textAbuse)
            report += "textabuse,";
        if (config->miscConfig.reportbot.griefing)
            report += "grief,";
        if (config->miscConfig.reportbot.wallhack)
            report += "wallhack,";
        if (config->miscConfig.reportbot.aimbot)
            report += "aimbot,";
        if (config->miscConfig.reportbot.other)
            report += "speedhack,";

        if (!report.empty()) {
            memory->submitReport(std::to_string(playerInfo.xuid).c_str(), report.c_str());
            lastReportTime = memory->globalVars->realtime;
            reportedPlayers.push_back(playerInfo.xuid);
        }
        return;
    }

    reportedPlayers.clear();
    ++reportbotRound;
}

void Misc::resetReportbot() noexcept {
    reportbotRound = 0;
    reportedPlayers.clear();
}

void Misc::preserveKillfeed(bool roundStart) noexcept {
    if (!config->miscConfig.preserveKillfeed.enabled)
        return;

    static auto nextUpdate = 0.0f;

    if (roundStart) {
        nextUpdate = memory->globalVars->realtime + 10.0f;
        return;
    }

    if (nextUpdate > memory->globalVars->realtime)
        return;

    nextUpdate = memory->globalVars->realtime + 2.0f;

    const auto deathNotice = std::uintptr_t(memory->findHudElement(memory->hud, "CCSGO_HudDeathNotice"));
    if (!deathNotice)
        return;

    const auto deathNoticePanel = (*(UIPanel**)(*reinterpret_cast<std::uintptr_t*>(deathNotice WIN32_LINUX(-20 + 88, -32 + 128)) + sizeof(std::uintptr_t)));

    const auto childPanelCount = deathNoticePanel->getChildCount();

    for (int i = 0; i < childPanelCount; ++i) {
        const auto child = deathNoticePanel->getChild(i);
        if (!child)
            continue;

        if (child->hasClass("DeathNotice_Killer") && (!config->miscConfig.preserveKillfeed.onlyHeadshots || child->hasClass("DeathNoticeHeadShot")))
            child->setAttributeFloat("SpawnTime", memory->globalVars->currenttime);
    }
}

void Misc::voteRevealer(GameEvent& event) noexcept {
    if (!config->miscConfig.revealVotes)
        return;

    const auto entity = interfaces->entityList->getEntity(event.getInt("entityid"));
    if (!entity || !entity->isPlayer())
        return;

    const auto votedYes = event.getInt("vote_option") == 0;
    const auto isLocal = localPlayer && entity == localPlayer.get();
    const char color = votedYes ? '\x06' : '\x07';

    memory->clientMode->getHudChat()->printf(0, " \x0C\u2022Osiris\u2022 %c%s\x01 voted %c%s\x01", isLocal ? '\x01' : color, isLocal ? "You" : entity->getPlayerName().c_str(), color, votedYes ? "Yes" : "No");
}

void Misc::onVoteStart(const void* data, int size) noexcept {
    if (!config->miscConfig.revealVotes)
        return;

    constexpr auto voteName = [](int index) {
        switch (index) {
            case 0: return "Kick";
            case 1: return "Change Level";
            case 6: return "Surrender";
            case 13: return "Start TimeOut";
            default: return "";
        }
    };

    const auto reader = ProtobufReader{ static_cast<const std::uint8_t*>(data), size };
    const auto entityIndex = reader.readInt32(2);

    const auto entity = interfaces->entityList->getEntity(entityIndex);
    if (!entity || !entity->isPlayer())
        return;

    const auto isLocal = localPlayer && entity == localPlayer.get();

    const auto voteType = reader.readInt32(3);
    memory->clientMode->getHudChat()->printf(0, " \x0C\u2022Osiris\u2022 %c%s\x01 call vote (\x06%s\x01)", isLocal ? '\x01' : '\x06', isLocal ? "You" : entity->getPlayerName().c_str(), voteName(voteType));
}

void Misc::onVotePass() noexcept {
    if (config->miscConfig.revealVotes)
        memory->clientMode->getHudChat()->printf(0, " \x0C\u2022Osiris\u2022\x01 Vote\x06 PASSED");
}

void Misc::onVoteFailed() noexcept {
    if (config->miscConfig.revealVotes)
        memory->clientMode->getHudChat()->printf(0, " \x0C\u2022Osiris\u2022\x01 Vote\x07 FAILED");
}

// ImGui::ShadeVertsLinearColorGradientKeepAlpha() modified to do interpolation in HSV
static void shadeVertsHSVColorGradientKeepAlpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1) {
    ImVec2 gradient_extent = gradient_p1 - gradient_p0;
    float gradient_inv_length2 = 1.0f / ImLengthSqr(gradient_extent);
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;

    ImVec4 col0HSV = ImGui::ColorConvertU32ToFloat4(col0);
    ImVec4 col1HSV = ImGui::ColorConvertU32ToFloat4(col1);
    ImGui::ColorConvertRGBtoHSV(col0HSV.x, col0HSV.y, col0HSV.z, col0HSV.x, col0HSV.y, col0HSV.z);
    ImGui::ColorConvertRGBtoHSV(col1HSV.x, col1HSV.y, col1HSV.z, col1HSV.x, col1HSV.y, col1HSV.z);
    ImVec4 colDelta = col1HSV - col0HSV;

    for (ImDrawVert* vert = vert_start; vert < vert_end; vert++) {
        float d = ImDot(vert->pos - gradient_p0, gradient_extent);
        float t = ImClamp(d * gradient_inv_length2, 0.0f, 1.0f);

        float h = col0HSV.x + colDelta.x * t;
        float s = col0HSV.y + colDelta.y * t;
        float v = col0HSV.z + colDelta.z * t;

        ImVec4 rgb;
        ImGui::ColorConvertHSVtoRGB(h, s, v, rgb.x, rgb.y, rgb.z);
        vert->col = (ImGui::ColorConvertFloat4ToU32(rgb) & ~IM_COL32_A_MASK) | (vert->col & IM_COL32_A_MASK);
    }
}

void Misc::drawOffscreenEnemies(ImDrawList* drawList) noexcept {
    if (!config->miscConfig.offscreenEnemies.enabled)
        return;

    const auto yaw = Helpers::deg2rad(interfaces->engine->getViewAngles().y);

    GameData::Lock lock;
    for (auto& player : GameData::players()) {
        if ((player.dormant && player.fadingAlpha() == 0.0f) || !player.alive || !player.enemy || player.inViewFrustum)
            continue;

        const auto positionDiff = GameData::local().origin - player.origin;

        auto x = std::cos(yaw) * positionDiff.y - std::sin(yaw) * positionDiff.x;
        auto y = std::cos(yaw) * positionDiff.x + std::sin(yaw) * positionDiff.y;
        if (const auto len = std::sqrt(x * x + y * y); len != 0.0f) {
            x /= len;
            y /= len;
        }

        constexpr auto avatarRadius = 13.0f;
        constexpr auto triangleSize = 10.0f;

        const auto pos = ImGui::GetIO().DisplaySize / 2 + ImVec2{ x, y } *200;
        const auto trianglePos = pos + ImVec2{ x, y } *(avatarRadius + (config->miscConfig.offscreenEnemies.healthBar.enabled ? 5 : 3));

        Helpers::setAlphaFactor(player.fadingAlpha());
        const auto white = Helpers::calculateColor(255, 255, 255, 255);
        const auto background = Helpers::calculateColor(0, 0, 0, 80);
        const auto color = Helpers::calculateColor(config->miscConfig.offscreenEnemies.asColor4());
        const auto healthBarColor = config->miscConfig.offscreenEnemies.healthBar.type == HealthBar::HealthBased ? Helpers::healthColor(std::clamp(player.health / 100.0f, 0.0f, 1.0f)) : Helpers::calculateColor(config->miscConfig.offscreenEnemies.healthBar.asColor4());
        Helpers::setAlphaFactor(1.0f);

        const ImVec2 trianglePoints[]{
            trianglePos + ImVec2{ 0.4f * y, -0.4f * x } *triangleSize,
            trianglePos + ImVec2{ 1.0f * x, 1.0f * y } *triangleSize,
            trianglePos + ImVec2{ -0.4f * y, 0.4f * x } *triangleSize
        };

        drawList->AddConvexPolyFilled(trianglePoints, 3, color);
        drawList->AddCircleFilled(pos, avatarRadius + 1, white & IM_COL32_A_MASK, 40);

        const auto texture = player.getAvatarTexture();

        const bool pushTextureId = drawList->_TextureIdStack.empty() || texture != drawList->_TextureIdStack.back();
        if (pushTextureId)
            drawList->PushTextureID(texture);

        const int vertStartIdx = drawList->VtxBuffer.Size;
        drawList->AddCircleFilled(pos, avatarRadius, white, 40);
        const int vertEndIdx = drawList->VtxBuffer.Size;
        ImGui::ShadeVertsLinearUV(drawList, vertStartIdx, vertEndIdx, pos - ImVec2{ avatarRadius, avatarRadius }, pos + ImVec2{ avatarRadius, avatarRadius }, { 0, 0 }, { 1, 1 }, true);

        if (pushTextureId)
            drawList->PopTextureID();

        if (config->miscConfig.offscreenEnemies.healthBar.enabled) {
            const auto radius = avatarRadius + 2;
            const auto healthFraction = std::clamp(player.health / 100.0f, 0.0f, 1.0f);

            drawList->AddCircle(pos, radius, background, 40, 3.0f);

            const int vertStartIdx = drawList->VtxBuffer.Size;
            if (healthFraction == 1.0f) { // sometimes PathArcTo is missing one top pixel when drawing a full circle, so draw it with AddCircle
                drawList->AddCircle(pos, radius, healthBarColor, 40, 2.0f);
            }
            else {
                constexpr float pi = std::numbers::pi_v<float>;
                drawList->PathArcTo(pos, radius - 0.5f, pi / 2 - pi * healthFraction, pi / 2 + pi * healthFraction, 40);
                drawList->PathStroke(healthBarColor, false, 2.0f);
            }
            const int vertEndIdx = drawList->VtxBuffer.Size;

            if (config->miscConfig.offscreenEnemies.healthBar.type == HealthBar::Gradient)
                shadeVertsHSVColorGradientKeepAlpha(drawList, vertStartIdx, vertEndIdx, pos - ImVec2{ 0.0f, radius }, pos + ImVec2{ 0.0f, radius }, IM_COL32(0, 255, 0, 255), IM_COL32(255, 0, 0, 255));
        }
    }
}

void Misc::autoAccept(const char* soundEntry) noexcept {
    if (!config->miscConfig.autoAccept)
        return;

    if (std::strcmp(soundEntry, "UIPanorama.popup_accept_match_beep"))
        return;

    if (const auto idx = memory->registeredPanoramaEvents->find(memory->makePanoramaSymbol("MatchAssistedAccept")); idx != -1) {
        if (const auto eventPtr = memory->registeredPanoramaEvents->memory[idx].value.makeEvent(nullptr))
            interfaces->panoramaUIEngine->accessUIEngine()->dispatchEvent(eventPtr);
    }

#ifdef _WIN32
    auto window = FindWindowW(L"Valve001", NULL);
    FLASHWINFO flash{ sizeof(FLASHWINFO), window, FLASHW_TRAY | FLASHW_TIMERNOFG, 0, 0 };
    FlashWindowEx(&flash);
    ShowWindow(window, SW_RESTORE);
#endif
}

void Misc::updateEventListeners(bool forceRemove) noexcept {
    class PurchaseEventListener : public GameEventListener {
    public:
        void fireGameEvent(GameEvent* event) override { purchaseList(event); }
    };

    static PurchaseEventListener listener;
    static bool listenerRegistered = false;

    if (config->miscConfig.purchaseList.enabled && !listenerRegistered) {
        interfaces->gameEventManager->addListener(&listener, "item_purchase");
        listenerRegistered = true;
    }
    else if ((!config->miscConfig.purchaseList.enabled || forceRemove) && listenerRegistered) {
        interfaces->gameEventManager->removeListener(&listener);
        listenerRegistered = false;
    }
}

void Misc::updateInput() noexcept {}