/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zmk/events/keycode_state_changed.h>
#include <string.h>

ZMK_EVENT_IMPL(zmk_keycode_state_changed);

static sys_slist_t active_events_registry = SYS_SLIST_STATIC_INIT(&active_events_registry);

struct zmk_keycode_state_changed_event *
zmk_keycode_state_changed_from_encoded(uint32_t encoded, bool pressed, int64_t timestamp) {

    uint16_t page = ZMK_HID_USAGE_PAGE(encoded);
    uint16_t id = ZMK_HID_USAGE_ID(encoded);
    uint8_t implicit_modifiers = 0x00;
    uint8_t explicit_modifiers = 0x00;

    if (!page) {
        page = HID_USAGE_KEY;
    }

    if (is_mod(page, id)) {
        explicit_modifiers = SELECT_MODS(encoded);
    } else {
        implicit_modifiers = SELECT_MODS(encoded);
    }

    bool event_found = false;
    bool event_exhausted = false;

    zmk_keycode_event_registry_item_t *event = NULL;
    SYS_SLIST_FOR_EACH_CONTAINER(&active_events_registry, event, node) {
        if (event->keycode == encoded) {
            event_found = true;
            if (pressed)
                event->event_count++;
            else if (event->event_count > 0) {
                event->event_count--;
            }

            if (event->event_count == 0)
                event_exhausted = true;

            break;
        }
    }

    if (event_exhausted) {
        sys_slist_find_and_remove(&active_events_registry, &event->node);
        k_free(event);
    }

    if (!event_found) {
        event = k_malloc(sizeof(zmk_keycode_event_registry_item_t));
        memset(event, 0, sizeof(zmk_keycode_event_registry_item_t));
        event->keycode = encoded;
        event->event_count = 1;
        sys_slist_append(&active_events_registry, &event->node);
    }

    if (!event_found ||
        event_exhausted) // send only first keypress and last release event of the same keycode

        return new_zmk_keycode_state_changed(
            (struct zmk_keycode_state_changed){.usage_page = page,
                                               .keycode = id,
                                               .implicit_modifiers = implicit_modifiers,
                                               .explicit_modifiers = explicit_modifiers,
                                               .state = pressed,
                                               .timestamp = timestamp});
    return NULL;
}