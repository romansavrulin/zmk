/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/matrix_transform.h>
#include <zmk/event_manager.h>
#include <zmk/key_merger.h>
#include <zmk/events/position_state_changed.h>
#include <string.h>

typedef struct zmk_scancode_event_registry_item {
    sys_snode_t node;
    uint32_t position;
    uint32_t event_count;
    bool pressed;
} zmk_scancode_event_registry_item_t;

static sys_slist_t active_keypress_registry = SYS_SLIST_STATIC_INIT(&active_keypress_registry);

bool zmk_key_merger_consume_event(uint32_t position, bool pressed) {
    int32_t orig_position = position; //unmerged position

    // tbd: add merge keys config instead hardcoded values

    if (position == 42)
        position = 39; //merge spaces between both sides

    LOG_INF("Orig_position: %d, merged_position: %d, pressed: %s", orig_position, position,
            (pressed ? "true" : "false"));

    bool event_found = false;
    bool event_exhausted = false;

    zmk_scancode_event_registry_item_t *event = NULL;
    SYS_SLIST_FOR_EACH_CONTAINER(&active_keypress_registry, event, node) {
        if( event->position == position ) {
            event_found = true;
            if( pressed )
                event->event_count++;
            else if (event->event_count > 0){
                event->event_count--;
            }

            if(event->event_count == 0)
                event_exhausted = true;

            break;
        }
    }

    if(event_exhausted){
        sys_slist_find_and_remove(&active_keypress_registry, &event->node);
        k_free(event);
    }

    if ( !event_found ){
        event = k_malloc(sizeof(zmk_scancode_event_registry_item_t));
        memset (event, 0, sizeof(zmk_scancode_event_registry_item_t));
        event->position = position;
        event->event_count = 1;
        sys_slist_append(&active_keypress_registry, &event->node);
    }

    if (event_found && !event_exhausted){ // send only first keypress and last release event of the same position
        LOG_INF("Merged key is already pressed. Consuming event"
            "orig_position: %d, merged_position: %d, event_count: %d, pressed: %s", 
            orig_position, position, event->event_count, (pressed ? "true" : "false"));
        return true;
    }

    LOG_DBG("Raising event for orig_position: %d, merged_position: %d, pressed: %s", orig_position, position,
            (pressed ? "true" : "false"));

    return false;
}
