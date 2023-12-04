/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

typedef struct zmk_scancode_event_registry_item {
    sys_snode_t node;
    uint32_t position;
    uint32_t event_count;
} zmk_scancode_event_registry_item_t;

int32_t zmk_matrix_transform_row_column_to_position(uint32_t row, uint32_t column);