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
#include <zmk/events/position_state_changed.h>
#include <string.h>

#define ZMK_KSCAN_EVENT_STATE_PRESSED 0
#define ZMK_KSCAN_EVENT_STATE_RELEASED 1

struct zmk_kscan_event {
    uint32_t row;
    uint32_t column;
    uint32_t state;
};

struct zmk_kscan_msg_processor {
    struct k_work work;
} msg_processor;

static sys_slist_t active_keypress_registry = SYS_SLIST_STATIC_INIT(&active_keypress_registry);

K_MSGQ_DEFINE(zmk_kscan_msgq, sizeof(struct zmk_kscan_event), CONFIG_ZMK_KSCAN_EVENT_QUEUE_SIZE, 4);

static void zmk_kscan_callback(const struct device *dev, uint32_t row, uint32_t column,
                               bool pressed) {
    struct zmk_kscan_event ev = {
        .row = row,
        .column = column,
        .state = (pressed ? ZMK_KSCAN_EVENT_STATE_PRESSED : ZMK_KSCAN_EVENT_STATE_RELEASED)};

    k_msgq_put(&zmk_kscan_msgq, &ev, K_NO_WAIT);
    k_work_submit(&msg_processor.work);
}

void zmk_kscan_process_msgq(struct k_work *item) {
    struct zmk_kscan_event ev;

    while (k_msgq_get(&zmk_kscan_msgq, &ev, K_NO_WAIT) == 0) {
        bool pressed = (ev.state == ZMK_KSCAN_EVENT_STATE_PRESSED);
        int32_t position = zmk_matrix_transform_row_column_to_position(ev.row, ev.column);
        int32_t orig_position = position; //unmerged position

        if (position < 0) {
            LOG_WRN("Not found in transform: row: %d, col: %d, pressed: %s", ev.row, ev.column,
                    (pressed ? "true" : "false"));
            continue;
        }

        //tbd: add merge keys config instead hardcoded values

        if(position == 56)
            position = 53; //42 is merged to 39 matrix position

        LOG_INF("Row: %d, col: %d, orig_position: %d, merged_position: %d, pressed: %s", ev.row, ev.column, orig_position, position, 
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

        if(event_found && !event_exhausted){ // send only first keypress and last release event of the same position
            LOG_INF("Merged key is already pressed. Row: %d, col: %d, "
                "orig_position: %d, merged_position: %d, event_count: %d, pressed: %s", 
                ev.row, ev.column, orig_position, position, event->event_count, (pressed ? "true" : "false"));
            continue;
        }

        LOG_DBG("Raising event for row: %d, col: %d, orig_position: %d, merged_position: %d, pressed: %s", ev.row, ev.column, orig_position, position,
                (pressed ? "true" : "false")); 
        
        ZMK_EVENT_RAISE(new_zmk_position_state_changed(
            (struct zmk_position_state_changed){.source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
                                                .state = pressed,
                                                .position = position,
                                                .timestamp = k_uptime_get()}));
    }
}

int zmk_kscan_init(const struct device *dev) {
    if (dev == NULL) {
        LOG_ERR("Failed to get the KSCAN device");
        return -EINVAL;
    }

    k_work_init(&msg_processor.work, zmk_kscan_process_msgq);

    kscan_config(dev, zmk_kscan_callback);
    kscan_enable_callback(dev);

    return 0;
}
