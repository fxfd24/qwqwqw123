#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include <lvgl.h>
#include <ui/ui.h>
#include "config.h"
#include "parent_control.h"


// External references to UI elements from ui.h
extern lv_obj_t *ui_wake_group;
extern lv_obj_t *ui_sleep_group;
extern lv_obj_t *ui_pnlMain;

// Static globals for this file
static lv_obj_t *ui_btnSettings = NULL;
static lv_obj_t *ui_pinKeypad = NULL;
static lv_obj_t *ui_pinTextArea = NULL;
static lv_obj_t *ui_errorMsg = NULL;

// Global state instance
ParentControlState parent_control;

// Forward declarations of static functions
static void settings_btn_event_cb(lv_event_t *e);
static void pin_keypad_event_cb(lv_event_t *e);


void show_error_message(const char* message) {
    if (ui_errorMsg == NULL) {
        ui_errorMsg = lv_label_create(ui_pnlMain);
        lv_obj_set_style_text_color(ui_errorMsg, lv_color_hex(0xFF0000), 0);
    }
    lv_label_set_text(ui_errorMsg, message);
    lv_obj_align(ui_errorMsg, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_clear_flag(ui_errorMsg, LV_OBJ_FLAG_HIDDEN);
    
    // Auto-hide error after 3 seconds
    static lv_timer_t* hide_timer = NULL;
    if (hide_timer) {
        lv_timer_reset(hide_timer);
    } else {
        hide_timer = lv_timer_create([](lv_timer_t* timer) {
            if (ui_errorMsg) {
                lv_obj_add_flag(ui_errorMsg, LV_OBJ_FLAG_HIDDEN);
            }
            lv_timer_del(timer);
        }, 3000, NULL);
    }
}

void init_parent_control() {
    parent_control = {
        .is_unlocked = false,
        .unlock_time = 0,
        .failed_attempts = 0,
        .lockout_until = 0,
        .settings_visible = false
    };
}

bool is_locked_out() {
    if (parent_control.lockout_until > millis()) {
        return true;
    }
    parent_control.lockout_until = 0;
    return false;
}



void show_settings_panel() {
    log_i("show_settings_panel called");
    
    if (parent_control.settings_visible) {
        log_w("Settings panel already visible");
        return;
    }
    
    if (!parent_control.is_unlocked) {
        log_w("Attempted to show settings while locked");
        return;
    }
    
    log_i("Setting panel to visible");
    parent_control.settings_visible = true;
    
    // Safely show UI elements
    if (!ui_wake_group || !lv_obj_is_valid(ui_wake_group)) {
        log_e("Invalid wake group object");
        return;
    }
    
    if (!ui_sleep_group || !lv_obj_is_valid(ui_sleep_group)) {
        log_e("Invalid sleep group object");
        return;
    }
    
    log_i("Showing wake group");
    lv_obj_clear_flag(ui_wake_group, LV_OBJ_FLAG_HIDDEN);
    
    log_i("Showing sleep group");
    lv_obj_clear_flag(ui_sleep_group, LV_OBJ_FLAG_HIDDEN);
    
    // Force a refresh of the UI
    lv_obj_invalidate(ui_pnlMain);
}

bool verify_pin(const char* entered_pin) {
    log_i("Verifying PIN");
    
    if (is_locked_out()) {
        log_w("System is currently locked out");
        return false;
    }
    
    if (!entered_pin) {
        log_e("Null PIN provided");
        return false;
    }
    
    log_i("Comparing PINs");
    bool is_valid = strcmp(entered_pin, PARENT_PIN) == 0;
    
    if (!is_valid) {
        parent_control.failed_attempts++;
        log_w("Invalid PIN attempt %d/%d", parent_control.failed_attempts, MAX_FAILED_ATTEMPTS);
        
        if (parent_control.failed_attempts >= MAX_FAILED_ATTEMPTS) {
            parent_control.lockout_until = millis() + LOCKOUT_DURATION_MS;
            parent_control.failed_attempts = 0;
            log_w("Max attempts reached, system locked out until %lu", parent_control.lockout_until);
        }
        return false;
    }
    
    log_i("PIN verified successfully");
    parent_control.failed_attempts = 0;
    parent_control.is_unlocked = true;
    parent_control.unlock_time = millis();
    return true;
}



void hide_settings_panel() {
    if (!parent_control.settings_visible) {
        return; // Already hidden
    }
    
    parent_control.settings_visible = false;
    
    // Safely hide UI elements
    if (ui_wake_group && lv_obj_is_valid(ui_wake_group)) {
        lv_obj_add_flag(ui_wake_group, LV_OBJ_FLAG_HIDDEN);
    }
    
    if (ui_sleep_group && lv_obj_is_valid(ui_sleep_group)) {
        lv_obj_add_flag(ui_sleep_group, LV_OBJ_FLAG_HIDDEN);
    }
    
    if (ui_pinKeypad && lv_obj_is_valid(ui_pinKeypad)) {
        lv_obj_add_flag(ui_pinKeypad, LV_OBJ_FLAG_HIDDEN);
    }
    
    if (ui_pinTextArea && lv_obj_is_valid(ui_pinTextArea)) {
        lv_obj_add_flag(ui_pinTextArea, LV_OBJ_FLAG_HIDDEN);
    }
}

void check_unlock_timeout() {
    if (!parent_control.is_unlocked) return;
    
    if (millis() - parent_control.unlock_time >= UNLOCK_TIMEOUT_MS) {
        parent_control.is_unlocked = false;
        hide_settings_panel();
    }
}


static void pin_keypad_event_cb(lv_event_t *e) {
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    if (!btn || !lv_obj_is_valid(btn)) {
        log_e("Invalid button object");
        return;
    }
    
    uint32_t id = lv_btnmatrix_get_selected_btn(btn);
    const char *txt = lv_btnmatrix_get_btn_text(btn, id);
    
    if (!txt || !ui_pinTextArea || !lv_obj_is_valid(ui_pinTextArea)) {
        log_e("Invalid text or text area");
        return;
    }
    
    log_i("Button pressed: %s", txt);
    
    if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
        const char* current_text = lv_textarea_get_text(ui_pinTextArea);
        if (current_text && strlen(current_text) > 0) {
            lv_textarea_set_text(ui_pinTextArea, "");
            if (strlen(current_text) > 1) {
                char temp[32];
                strncpy(temp, current_text, strlen(current_text) - 1);
                temp[strlen(current_text) - 1] = '\0';
                lv_textarea_set_text(ui_pinTextArea, temp);
            }
        }
    } 
    else if (strcmp(txt, "Enter") == 0) {
        const char *pin = lv_textarea_get_text(ui_pinTextArea);
        if (!pin) {
            log_e("No PIN entered");
            return;
        }
        
        log_i("Verifying PIN");
        if (verify_pin(pin)) {
            log_i("PIN verified successfully");
            
            // First hide the keypad and text area
            if (ui_pinKeypad && lv_obj_is_valid(ui_pinKeypad)) {
                log_i("Hiding keypad");
                lv_obj_add_flag(ui_pinKeypad, LV_OBJ_FLAG_HIDDEN);
            }
            if (ui_pinTextArea && lv_obj_is_valid(ui_pinTextArea)) {
                log_i("Hiding text area");
                lv_obj_add_flag(ui_pinTextArea, LV_OBJ_FLAG_HIDDEN);
            }
            
            // Then show the settings panel
            log_i("Showing settings panel");
            parent_control.is_unlocked = true;
            parent_control.unlock_time = millis();
            show_settings_panel();
        } else {
            log_w("Invalid PIN entered");
            lv_textarea_set_text(ui_pinTextArea, "");
            if (is_locked_out()) {
                char buf[64];
                int minutes = (parent_control.lockout_until - millis()) / 60000;
                snprintf(buf, sizeof(buf), "Locked out. Try again in %d minutes", minutes);
                show_error_message(buf);
            } else {
                show_error_message("Invalid PIN");
            }
        }
    } 
    else {
        const char* current_text = lv_textarea_get_text(ui_pinTextArea);
        if (current_text && strlen(current_text) < 8) {  // Limit PIN length
            lv_textarea_add_text(ui_pinTextArea, txt);
        }
    }
}



void create_pin_keypad() {
    ui_pinTextArea = lv_textarea_create(ui_pnlMain);
    if (ui_pinTextArea == NULL) return;
    
    lv_obj_set_size(ui_pinTextArea, 200, 50);
    lv_obj_align(ui_pinTextArea, LV_ALIGN_TOP_MID, 0, 50);
    lv_textarea_set_text(ui_pinTextArea, "");
    lv_textarea_set_password_mode(ui_pinTextArea, true);
    lv_textarea_set_one_line(ui_pinTextArea, true);
    lv_obj_add_flag(ui_pinTextArea, LV_OBJ_FLAG_HIDDEN);
    
    static const char* kb_map[] = {
        "1", "2", "3", "\n",
        "4", "5", "6", "\n",
        "7", "8", "9", "\n",
        LV_SYMBOL_BACKSPACE, "0", "Enter", ""
    };
    
    ui_pinKeypad = lv_btnmatrix_create(ui_pnlMain);
    if (ui_pinKeypad == NULL) return;
    
    lv_obj_set_size(ui_pinKeypad, 200, 200);
    lv_obj_align(ui_pinKeypad, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_add_flag(ui_pinKeypad, LV_OBJ_FLAG_HIDDEN);
    
    lv_btnmatrix_set_map(ui_pinKeypad, kb_map);
    lv_obj_add_event_cb(ui_pinKeypad, pin_keypad_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

void create_settings_button() {
    if (ui_btnSettings != NULL) {
        log_i("Settings button already exists");
        return;
    }
    
    ui_btnSettings = lv_btn_create(ui_pnlMain);
    if (ui_btnSettings == NULL) {
        log_e("Failed to create settings button");
        return;
    }
    
    log_i("Created settings button");
    
    lv_obj_set_size(ui_btnSettings, 50, 50);
    lv_obj_align(ui_btnSettings, LV_ALIGN_TOP_RIGHT, -10, 10);
    
    lv_obj_t *label = lv_label_create(ui_btnSettings);
    if (label == NULL) {
        log_e("Failed to create settings button label");
        return;
    }
    
    lv_label_set_text(label, LV_SYMBOL_SETTINGS);
    lv_obj_center(label);
    
    log_i("Settings button initialization complete");
    lv_obj_add_event_cb(ui_btnSettings, settings_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

static void settings_btn_event_cb(lv_event_t *e) {
    log_i("Settings button clicked");
    
    if (parent_control.settings_visible) {
        log_i("Hiding settings panel");
        hide_settings_panel();
        return;
    }
    
    if (!parent_control.is_unlocked) {
        log_i("Showing PIN entry");
        if (ui_pinKeypad && lv_obj_is_valid(ui_pinKeypad)) {
            lv_obj_clear_flag(ui_pinKeypad, LV_OBJ_FLAG_HIDDEN);
        }
        if (ui_pinTextArea && lv_obj_is_valid(ui_pinTextArea)) {
            lv_obj_clear_flag(ui_pinTextArea, LV_OBJ_FLAG_HIDDEN);
            lv_textarea_set_text(ui_pinTextArea, "");
        }
        return;
    }
    
    log_i("Showing settings panel");
    show_settings_panel();
}
