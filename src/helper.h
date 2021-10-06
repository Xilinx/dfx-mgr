/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	helper.h
 * @brief	helper types functions definition.
 */

#ifndef _ACAPD_HELPER_H
#define _ACAPD_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

typedef struct acapd_list {
	struct acapd_list *next, *prev;
}acapd_list_t;

#define ACAPD_INIT_LIST(name) { .next = &name, .prev = &name }
#define ACAPD_DECLARE_LIST(name)			\
	acapd_list_t name = ACAPD_INIT_LIST(name)

static inline void acapd_list_init(acapd_list_t *list)
{
	list->next = list->prev = list;
}

static inline void acapd_list_add_before(acapd_list_t *node,
					 acapd_list_t *new_node)
{
	new_node->prev = node->prev;
	new_node->next = node;
	new_node->next->prev = new_node;
	new_node->prev->next = new_node;
}

static inline void acapd_list_add_after(acapd_list_t *node,
					acapd_list_t *new_node)
{
	new_node->prev = node;
	new_node->next = node->next;
	new_node->next->prev = new_node;
	new_node->prev->next = new_node;
}

static inline void acapd_list_add_head(acapd_list_t *list,
				       acapd_list_t *node)
{
	acapd_list_add_after(list, node);
}

static inline void acapd_list_add_tail(acapd_list_t *list,
				       acapd_list_t *node)
{
	acapd_list_add_before(list, node);
}

static inline int acapd_list_is_empty(acapd_list_t *list)
{
	return list->next == list;
}

static inline void acapd_list_del(acapd_list_t *node)
{
	node->next->prev = node->prev;
	node->prev->next = node->next;
	node->next = node->prev = node;
}

static inline acapd_list_t *acapd_list_first(acapd_list_t *list)
{
	return acapd_list_is_empty(list) ? NULL : list->next;
}

#define acapd_list_for_each(list, node)		\
	for ((node) = (list)->next;		\
	     (node) != (list);			\
	     (node) = (node)->next)

#define offset_of(structure, member)    \
    ((uintptr_t)&(((structure *)0)->member))

#define acapd_container_of(ptr, structure, member)  \
    (void *)((uintptr_t)(ptr) - offset_of(structure, member))

/** Align 'size' down to a multiple of 'align' (must be a power of two). */
#define acapd_align_down(size, align)			\
	((size) & ~((align) - 1))

/** Align 'size' up to a multiple of 'align' (must be a power of two). */
#define acapd_align_up(size, align)			\
	acapd_align_down((size) + (align) - 1, align)

#ifdef __cplusplus
}
#endif

#endif /*  _ACAPD_HELPER_H */
