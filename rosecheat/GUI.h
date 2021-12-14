#pragma once

#include <memory>
#include <vector>

struct ImFont;

class GUI {
public:
    GUI() noexcept;
    void render() noexcept;
    void handleToggle() noexcept;
    [[nodiscard]] bool isOpen() const noexcept { return open; }
private:
    bool open = true;

    void updateColors() const noexcept;
    void renderMenuBar() noexcept;

    void renderAimbotWindow(bool contentOnly = false) noexcept;
    void renderTriggerbotWindow(bool contentOnly = false) noexcept;
    void renderBacktrackWindow(bool contentOnly = false) noexcept;
    //void renderAntiAimWindow(bool contentOnly = false) noexcept; TODO
    void renderGlowWindow(bool contentOnly = false) noexcept;
    void renderChamsWindow(bool contentOnly = false) noexcept;
    void renderESPWindow(bool contentOnly = false) noexcept;
    void renderVisualsWindow(bool contentOnly = false) noexcept;
    void renderMiscWindow(bool contentOnly = false) noexcept;
    void renderPlayerListWindow(bool contentOnly = false) noexcept;
    void renderStyleWindow(bool contentOnly = false) noexcept;
    void renderConfigWindow(bool contentOnly = false) noexcept;

    struct {
        bool aimbot = false;
        bool triggerbot = false;
        bool backtrack = false;
        bool antiAim = false;
        bool glow = false;
        bool chams = false;
        bool esp = false;
        bool visuals = false;
        bool misc = false;
        bool playerList = false;
        bool style = false;
        bool config = false;
    } window;

    struct {
        ImFont* normal15px = nullptr;
    } fonts;

    float timeToNextConfigRefresh = 0.1f;
};

inline std::unique_ptr<GUI> gui;
