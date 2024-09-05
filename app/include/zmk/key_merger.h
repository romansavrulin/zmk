/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/devicetree.h>

#if DT_HAS_CHOSEN(zmk_key_merger)

#define ZMK_KEY_MERGER_NODE DT_CHOSEN(zmk_key_merger)
#define ZMK_KEY_MERGER_MAP_LEN DT_PROP_LEN(ZMK_KEY_MERGER_NODE, map)

#endif /* DT_HAS_CHOSEN(zmk_key_merger) */

bool zmk_key_merger_consume_event(uint32_t *event_position, bool pressed);
