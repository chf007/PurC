/**
 * @file variant-array.c
 * @author Xu Xiaohong (freemine)
 * @date 2021/07/08
 * @brief The API for variant.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include "config.h"
#include "private/variant.h"
#include "private/errors.h"
#include "variant-internals.h"
#include "purc-errors.h"
#include "purc-utils.h"

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static inline void
grown(purc_variant_t array, purc_variant_t value)
{
    purc_variant_t vals[] = { value };

    pcvariant_on_post_fired(array, pcvariant_atom_grow,
            PCA_TABLESIZE(vals), vals);
}

static inline void
shrunk(purc_variant_t array, purc_variant_t value)
{
    purc_variant_t vals[] = { value };

    pcvariant_on_post_fired(array, pcvariant_atom_shrink,
            PCA_TABLESIZE(vals), vals);
}

static inline void
change(purc_variant_t array,
        purc_variant_t o, purc_variant_t n)
{
    purc_variant_t vals[] = { o, n };

    pcvariant_on_post_fired(array, pcvariant_atom_change,
            PCA_TABLESIZE(vals), vals);
}

static void
arr_node_destroy(struct arr_node *node)
{
    if (node) {
        PURC_VARIANT_SAFE_CLEAR(node->val);
        free(node);
    }
}

static int
variant_arr_insert_before(variant_arr_t arr, size_t idx, purc_variant_t val)
{
    struct arr_node *node;
    node = (struct arr_node*)calloc(1, sizeof(*node));
    if (!node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    node->val = val;
    purc_variant_ref(val);

    struct pcutils_array_list *al = &arr->al;
    int r = pcutils_array_list_insert_before(al, idx, &node->al_node);
    if (r) {
        arr_node_destroy(node);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    return 0;
}

static int
variant_arr_append(variant_arr_t arr, purc_variant_t val)
{
    struct pcutils_array_list *al = &arr->al;
    size_t nr = pcutils_array_list_length(al);
    return variant_arr_insert_before(arr, nr, val);
}

static int
variant_arr_prepend(variant_arr_t arr, purc_variant_t val)
{
    return variant_arr_insert_before(arr, 0, val);
}

static purc_variant_t
variant_arr_get(variant_arr_t arr, size_t idx)
{
    struct pcutils_array_list *al = &arr->al;
    struct pcutils_array_list_node *p;
    p = pcutils_array_list_get(al, idx);
    struct arr_node *node;
    node = (struct arr_node*)container_of(p, struct arr_node, al_node);
    return node->val;
}

static int
variant_arr_set(variant_arr_t arr, size_t idx, purc_variant_t val)
{
    struct pcutils_array_list *al = &arr->al;
    struct pcutils_array_list_node *p;
    p = pcutils_array_list_get(al, idx);
    if (p == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    struct arr_node *node;
    node = (struct arr_node*)calloc(1, sizeof(*node));
    if (!node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    node->val = val;
    purc_variant_ref(val);

    struct pcutils_array_list_node *old;
    int r = pcutils_array_list_set(al, idx, &node->al_node, &old);
    if (r) {
        arr_node_destroy(node);
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    PC_ASSERT(p == old);
    if (old) {
        struct arr_node *node;
        node = (struct arr_node*)container_of(old, struct arr_node, al_node);
        arr_node_destroy(node);
    }

    return 0;
}

static int
variant_arr_remove(variant_arr_t arr, size_t idx)
{
    struct pcutils_array_list *al = &arr->al;

    struct pcutils_array_list_node *p;
    int r = pcutils_array_list_remove(al, idx, &p);
    if (r) {
        pcinst_set_error(PURC_ERROR_OVERFLOW);
        return -1;
    }
    if (p) {
        struct arr_node *node;
        node = (struct arr_node*)container_of(p, struct arr_node, al_node);
        arr_node_destroy(node);
    }

    return 0;
}

static size_t
variant_arr_length(variant_arr_t arr)
{
    struct pcutils_array_list *al = &arr->al;
    return pcutils_array_list_length(al);
}

static inline void
array_release (purc_variant_t value)
{
    variant_arr_t data = (variant_arr_t)value->sz_ptr[1];
    if (!data)
        return;

    struct pcutils_array_list *al = &data->al;
    struct arr_node *p, *n;
    array_list_for_each_entry_reverse_safe(al, p, n, al_node) {
        struct pcutils_array_list_node *node;
        int r = pcutils_array_list_remove(al, p->al_node.idx, &node);
        PC_ASSERT(r==0 && node && node == &p->al_node);
        arr_node_destroy(p);
    };

    pcutils_array_list_reset(al);
    free(data);
    value->sz_ptr[1] = (uintptr_t)NULL;

    pcvariant_stat_set_extra_size(value, 0);
}

static void
refresh_extra(purc_variant_t array)
{
    size_t extra = 0;
    variant_arr_t arr = (variant_arr_t)array->sz_ptr[1];
    if (arr) {
        extra += sizeof(*arr);
        struct pcutils_array_list *al = &arr->al;
        extra += al->sz * sizeof(*al->nodes);
        extra += al->nr * sizeof(struct arr_node);
    }
    pcvariant_stat_set_extra_size(array, extra);
}

static purc_variant_t
pv_make_array_n (size_t sz, purc_variant_t value0, va_list ap)
{
    PCVARIANT_CHECK_FAIL_RET((sz==0 && value0==NULL) || (sz > 0 && value0),
        PURC_VARIANT_INVALID);

    purc_variant_t var = pcvariant_get(PVT(_ARRAY));
    if (!var) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    do {
        var->type          = PVT(_ARRAY);
        var->flags         = PCVARIANT_FLAG_EXTRA_SIZE;
        var->refc          = 1;

        size_t initial_size = ARRAY_LIST_DEFAULT_SIZE;
        if (sz>initial_size)
            initial_size = sz;

        variant_arr_t data = (variant_arr_t)calloc(1, sizeof(*data));
        if (!data) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }

        struct pcutils_array_list *al;
        al = &data->al;
        if (pcutils_array_list_init(al) ||
            pcutils_array_list_expand(al, initial_size))
        {
            free(data);
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }

        var->sz_ptr[1]     = (uintptr_t)data;

        if (sz > 0) {
            purc_variant_t v = value0;
            // question: shall we track mem for al->array?
            if (variant_arr_append(data, v)) {
                pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
                break;
            }

            size_t i = 1;
            while (i < sz) {
                v = va_arg(ap, purc_variant_t);
                if (!v) {
                    pcinst_set_error(PURC_ERROR_INVALID_VALUE);
                    break;
                }

                if (variant_arr_append(data, v)) {
                    pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
                    break;
                }

                i++;
            }

            if (i < sz)
                break;
        }

        refresh_extra(var);

        return var;

    } while (0);

    array_release(var);
    pcvariant_put(var);

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_variant_make_array (size_t sz, purc_variant_t value0, ...)
{
    purc_variant_t v;
    va_list ap;
    va_start(ap, value0);
    v = pv_make_array_n(sz, value0, ap);
    va_end(ap);

    return v;
}

void pcvariant_array_release (purc_variant_t value)
{
    array_release(value);
}

/* VWNOTE: unnecessary
int pcvariant_array_compare (purc_variant_t lv, purc_variant_t rv)
{
    // only called via purc_variant_compare
    struct pcutils_arrlist *lal = (struct pcutils_arrlist*)lv->sz_ptr[1];
    struct pcutils_arrlist *ral = (struct pcutils_arrlist*)rv->sz_ptr[1];
    size_t                  lnr = pcutils_arrlist_length(lal);
    size_t                  rnr = pcutils_arrlist_length(ral);

    size_t i = 0;
    for (; i<lnr && i<rnr; ++i) {
        purc_variant_t l = (purc_variant_t)lal->array[i];
        purc_variant_t r = (purc_variant_t)ral->array[i];
        int t = pcvariant_array_compare(l, r);
        if (t)
            return t;
    }

    return i<lnr ? 1 : -1;
}
*/

bool purc_variant_array_append (purc_variant_t array, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) && value,
        PURC_VARIANT_INVALID);

    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];
    int r = variant_arr_append(data, value);
    refresh_extra(array);
    return r ? false : true;
}

bool purc_variant_array_prepend (purc_variant_t array, purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) && value,
        PURC_VARIANT_INVALID);

    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];
    int r = variant_arr_prepend(data, value);
    refresh_extra(array);
    return r ? false : true;
}

purc_variant_t purc_variant_array_get (purc_variant_t array, int idx)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) && idx>=0,
        PURC_VARIANT_INVALID);

    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];

    return variant_arr_get(data, idx);
}

bool purc_variant_array_size(purc_variant_t array, size_t *sz)
{
    PC_ASSERT(array && sz);

    PCVARIANT_CHECK_FAIL_RET(array->type==PVT(_ARRAY), false);

    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];
    *sz = variant_arr_length(data);
    return true;
}

bool purc_variant_array_set (purc_variant_t array, int idx,
        purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) &&
        idx>=0 && value && array != value,
        PURC_VARIANT_INVALID);

    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];
    int r = variant_arr_set(data, idx, value);
    refresh_extra(array);
    return r ? false : true;
    // size_t             nr = pcutils_arrlist_length(al);
    // if ((size_t)idx>=nr) {
    //     int t = pcutils_arrlist_put_idx(al, idx, value);

    //     if (t) {
    //         pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
    //         return false;
    //     }
    //     // fill empty slot with undefined value
    //     _fill_empty_with_undefined(al);
    //     // above two steps might be combined into one for better performance

    //     // since value is put into array
    //     purc_variant_ref(value);

    //     grown(array, value);

    //     size_t extra = sizeof(*al) + al->size * sizeof(*al->array);
    //     pcvariant_stat_set_extra_size(array, extra);

    //     return true;
    // } else {
    //     purc_variant_t v = (purc_variant_t)al->array[idx];
    //     if (v!=value) {
    //         change(array, v, value);
    //         purc_variant_unref(v);
    //         al->array[idx] = value;
    //     }
    //     purc_variant_ref(value);
    //     return true;
    // }
}

bool purc_variant_array_remove (purc_variant_t array, int idx)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) && idx>=0,
        PURC_VARIANT_INVALID);

    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];
    int r = variant_arr_remove(data, idx);
    refresh_extra(array);
    return r ? false : true;
    // size_t             nr = pcutils_arrlist_length(al);
    // if ((size_t)idx>=nr)
    //     return true; // or false?

    // purc_variant_t v = (purc_variant_t)al->array[idx];
    // // pcutils_arrlist_del_idx will shrink internally
    // if (pcutils_arrlist_del_idx(al, idx, 1)) {
    //     pcinst_set_error(PURC_ERROR_INVALID_VALUE);
    //     return false;
    // }

    // shrunk(array, v);

    // purc_variant_unref(v);

    // size_t extra = sizeof(*al) + al->size * sizeof(*al->array);
    // pcvariant_stat_set_extra_size(array, extra);

    // return true;
}

bool purc_variant_array_insert_before (purc_variant_t array, int idx,
        purc_variant_t value)
{
    PCVARIANT_CHECK_FAIL_RET(array && array->type==PVT(_ARRAY) &&
        idx>=0 && value && array != value,
        PURC_VARIANT_INVALID);

    variant_arr_t data = (variant_arr_t)array->sz_ptr[1];
    int r = variant_arr_insert_before(data, idx, value);
    refresh_extra(array);
    return r ? false : true;
    // struct pcutils_arrlist *al = (struct pcutils_arrlist*)array->sz_ptr[1];
    // size_t             nr = pcutils_arrlist_length(al);
    // if ((size_t)idx>=nr)
    //     idx = (int)nr;

    // // expand by 1 empty slot
    // if (pcutils_arrlist_shrink(al, 1)) {
    //     pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
    //     return false;
    // }

    // if ((size_t)idx<nr) {
    //     // move idx~nr-1 to idx+1~nr
    //     // pcutils_arrlist has no such api, we have to hack it whatsoever
    //     // note: overlap problem? man or test!
    //     memmove(al->array + idx + 1,
    //             al->array + idx,
    //             (nr-idx) * sizeof(void *));
    // }
    // al->array[idx] = value;
    // al->length    += 1;

    // shrunk(array, value);

    // purc_variant_ref(value);

    // size_t extra = sizeof(*al) + al->size * sizeof(*al->array);
    // pcvariant_stat_set_extra_size(array, extra);

    // return true;
}

bool purc_variant_array_insert_after (purc_variant_t array, int idx,
        purc_variant_t value)
{
    return purc_variant_array_insert_before(array, idx+1, value);
}

struct arr_user_data {
    int (*cmp)(purc_variant_t l, purc_variant_t r, void *ud);
    void *ud;
};

static int
sort_cmp(struct pcutils_array_list_node *l, struct pcutils_array_list_node *r,
        void *ud)
{
    struct arr_node *l_n, *r_n;
    l_n = container_of(l, struct arr_node, al_node);
    r_n = container_of(r, struct arr_node, al_node);

    struct arr_user_data *d = (struct arr_user_data*)ud;
    return d->cmp(l_n->val, r_n->val, d->ud);
}

int pcvariant_array_sort(purc_variant_t value, void *ud,
        int (*cmp)(purc_variant_t l, purc_variant_t r, void *ud))
{
    if (!value || value->type != PURC_VARIANT_TYPE_ARRAY)
        return -1;

    variant_arr_t data = (variant_arr_t)value->sz_ptr[1];

    struct arr_user_data d = {
        .cmp = cmp,
        .ud  = ud,
    };

    int r;
    r = pcutils_array_list_sort(&data->al, &d, sort_cmp);

    return r ? -1 : 0;
}

