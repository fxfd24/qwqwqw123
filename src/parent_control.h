#pragma once

#include <Arduino.h>

// Constants for parent control settings
constexpr uint16_t UNLOCK_TIMEOUT_MS = 30000;     // 30 second timeout
constexpr uint8_t MAX_FAILED_ATTEMPTS = 3;        // Lock out after 3 failed attempts
constexpr uint32_t LOCKOUT_DURATION_MS = 300000;  // 5 minute lockout

// Structure definition for parent control state
struct ParentControlState {
    bool is_unlocked;
    uint32_t unlock_time;
    uint8_t failed_attempts;
    uint32_t lockout_until;
    bool settings_visible;
};

// Public interface
void init_parent_control();
bool is_locked_out();
bool verify_pin(const char* entered_pin);
void check_unlock_timeout();
void create_settings_button();
void create_pin_keypad();
void show_settings_panel();
void hide_settings_panel();
void show_error_message(const char* message);