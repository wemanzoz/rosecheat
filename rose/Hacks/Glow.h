#pragma once

#include "../JsonForward.h"

#define OSIRIS_GLOW() true

namespace Glow {
    void render() noexcept;
    void clearCustomObjects() noexcept;
    void updateInput() noexcept;
}
