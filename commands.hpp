#pragma once

namespace tec {

    /// @brief Команды управления 
    enum class Commands : uint8_t {
        Nop = 0,
        VoltageGetSet = 0x04,
        Telemetry = 0x55,
        Invalid = 0xFF
    };

} // namespace tec
