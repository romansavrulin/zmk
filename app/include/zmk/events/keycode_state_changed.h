/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/keys.h>

struct zmk_keycode_state_changed {
    uint16_t usage_page;
    uint32_t keycode;
    uint8_t implicit_modifiers;
    uint8_t explicit_modifiers;
    bool state;
    int64_t timestamp;
};

typedef struct zmk_keycode_event_registry_item {
    sys_snode_t node;
    uint32_t keycode;
    uint32_t event_count;
} zmk_keycode_event_registry_item_t;

ZMK_EVENT_DECLARE(zmk_keycode_state_changed);

struct zmk_keycode_state_changed_event *
zmk_keycode_state_changed_from_encoded(uint32_t encoded, bool pressed, int64_t timestamp);
