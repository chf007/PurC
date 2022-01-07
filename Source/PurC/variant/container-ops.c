/**
 * @file container-ops.c
 * @author Xue Shuming
 * @date 2022/01/05
 * @brief The API for container ops.
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

#define MIN_BUFFER     512
#define MAX_BUFFER     1024 * 1024 * 1024

#define PRINT_VARIANT(v)                                                    \
    do {                                                                    \
        purc_rwstream_t rws = purc_rwstream_new_buffer(MIN_BUFFER,          \
                MAX_BUFFER);                                                \
        size_t len_expected = 0;                                            \
        purc_variant_serialize(v, rws,                                      \
                0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);           \
        char* buf = (char*)purc_rwstream_get_mem_buffer_ex(rws, NULL, NULL, \
                true);                                                      \
        fprintf(stderr, "variant=%s\n", buf);                               \
        free(buf);                                                          \
        purc_rwstream_destroy(rws);                                         \
    } while (0)

#define SET_SILENT_ERROR(error)                                     \
    do {                                                            \
        if (!silently)                                              \
        {                                                           \
            pcinst_set_error(error);                                \
        }                                                           \
    } while (0)


struct complex_ctxt {
    uintptr_t ctxt;
    uintptr_t extra;
};

// object member = key,   member_extra = value
// array  member = value, member_extra = PURC_VARIANT_INVALID
// set    member = value, member_extra = PURC_VARIANT_INVALID
typedef bool (*foreach_func)(void* ctxt, purc_variant_t member,
        purc_variant_t member_extra, bool silently);


// It is unsafe for func to remove the member
static bool
object_foreach(purc_variant_t object, foreach_func func, void* ctxt,
        bool silently)
{
    bool ret = false;
    size_t sz = purc_variant_object_get_size(object);
    if (sz == 0 || sz == PURC_VARIANT_BADSIZE) {
        goto end;
    }

    purc_variant_t k, v;
    foreach_key_value_in_variant_object(object, k, v)
        if (!func(ctxt, k, v, silently)) {
            goto end;
        }
    end_foreach;
    ret = true;

end:
    return ret;
}

// It is unsafe for func to remove the member
static bool
array_foreach(purc_variant_t array, foreach_func func, void* ctxt,
        bool silently)
{
    bool ret = false;
    size_t sz = purc_variant_array_get_size(array);
    if (sz == 0 || sz == PURC_VARIANT_BADSIZE) {
        goto end;
    }

    purc_variant_t val;
    foreach_value_in_variant_array(array, val)
        if (!func(ctxt, val, PURC_VARIANT_INVALID, silently)) {
            goto end;
        }
    end_foreach;
    ret = true;

end:
    return ret;
}

// It is unsafe for func to remove the member
bool
array_reverse_foreach(purc_variant_t array, foreach_func func, void* ctxt,
        bool silently)
{
    bool ret = false;
    size_t sz = purc_variant_array_get_size(array);
    if (sz == 0 || sz == PURC_VARIANT_BADSIZE) {
        goto end;
    }

    purc_variant_t val;
    struct pcutils_arrlist* al= (struct pcutils_arrlist*)array->sz_ptr[1];
    for (size_t i = al->length; i != 0; i--) {
        val = (purc_variant_t)al->array[i-1];
        if (!func(ctxt, val, (void*)(uintptr_t)(i - 1), silently)) {
            goto end;
        }
    }
    ret = true;

end:
    return ret;
}

// It is unsafe for func to remove the member
static bool
set_foreach(purc_variant_t set, foreach_func func, void* ctxt, bool silently)
{
    bool ret = false;
    size_t sz = purc_variant_set_get_size(set);
    if (sz == 0 || sz == PURC_VARIANT_BADSIZE) {
        goto end;
    }

    purc_variant_t v;
    foreach_value_in_variant_set(set, v)
        if (!func(ctxt, v, PURC_VARIANT_INVALID, silently)) {
            goto end;
        }
    end_foreach;
    ret = true;

end:
    return ret;
}

static bool
variant_object_clear(purc_variant_t object, bool silently)
{
    bool ret = false;
    if (object == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (!purc_variant_is_object(object)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    purc_variant_t key;
    purc_variant_t value;
    UNUSED_VARIABLE(value);
    foreach_in_variant_object_safe_x(object, key, value)
        if (!purc_variant_object_remove(object, key)) {
            goto end;
        }
    end_foreach;
    ret = true;

end:
    return ret;
}

static bool
variant_array_clear(purc_variant_t array, bool silently)
{
    bool ret = false;
    if (array == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (!purc_variant_is_array(array)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    purc_variant_t val;
    size_t curr;
    UNUSED_VARIABLE(val);
    foreach_value_in_variant_array_safe(array, val, curr)
        if (!purc_variant_array_remove(array, curr)) {
            goto end;
        }
        --curr;
    end_foreach;
    ret = true;

end:
    return ret;
}

static bool
variant_set_clear(purc_variant_t set, bool silently)
{
    bool ret = false;
    if (set == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (!purc_variant_is_set(set)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    purc_variant_t v;
    struct rb_node *n;
    UNUSED_VARIABLE(n);
    foreach_value_in_variant_set_safe_x(set, v, n)
        if (!purc_variant_set_remove(set, v)) {
            goto end;
        }
    end_foreach;
    ret = true;

end:
    return ret;
}

static bool
is_in_set(purc_variant_t set, purc_variant_t v)
{
    bool ret = false;
    purc_variant_t val;
    foreach_value_in_variant_set(set, val)
        if (purc_variant_compare_ex(val, v, PCVARIANT_COMPARE_OPT_AUTO) == 0) {
            ret = true;
            goto end;
        }
    end_foreach;

end:
    return ret;
}

static bool
is_in_array(purc_variant_t array, purc_variant_t v, int* idx)
{
    bool ret = false;
    purc_variant_t val;
    size_t curr;
    UNUSED_VARIABLE(val);
    foreach_value_in_variant_array_safe(array, val, curr)
        if (purc_variant_compare_ex(val, v, PCVARIANT_COMPARE_OPT_AUTO) == 0) {
            if (idx) {
                *idx = curr;
            }
            ret = true;
            goto end;
        }
    end_foreach;

end:
    return ret;
}


static bool
add_object_member(void* dst, purc_variant_t key,
        purc_variant_t value, bool silently)
{
    UNUSED_PARAM(silently);
    return purc_variant_object_set((purc_variant_t)dst, key, value);
}

static bool
remove_object_member(void* dst, purc_variant_t key, purc_variant_t value,
        bool silently)
{
    UNUSED_PARAM(value);
    UNUSED_PARAM(silently);
    // TODO: silently
    return purc_variant_object_remove((purc_variant_t)dst, key);
}

static bool
append_array_member(void* ctxt, purc_variant_t member,
        purc_variant_t member_extra, bool silently)
{
    UNUSED_PARAM(member_extra);
    UNUSED_PARAM(silently);
    return purc_variant_array_append((purc_variant_t)ctxt, member);
}

static bool
remove_array_member(void* ctxt, purc_variant_t member,
        purc_variant_t member_extra, bool silently)
{
    UNUSED_PARAM(member_extra);
    UNUSED_PARAM(silently);
    int idx = -1;
    purc_variant_t array = (purc_variant_t) ctxt;
    if(is_in_array(array, member, &idx)) {
        return purc_variant_array_remove(array, idx);
    }
    return true;
}

static bool
prepend_array_member(void* ctxt, purc_variant_t member,
        purc_variant_t member_extra, bool silently)
{
    UNUSED_PARAM(member_extra);
    UNUSED_PARAM(silently);
    return purc_variant_array_prepend((purc_variant_t)ctxt, member);
}

static bool
insert_before_array_member(void* ctxt, purc_variant_t member,
        purc_variant_t member_extra, bool silently)
{
    UNUSED_PARAM(member_extra);
    UNUSED_PARAM(silently);
    struct complex_ctxt* c_ctxt = (struct complex_ctxt*) ctxt;
    purc_variant_t array = (purc_variant_t) c_ctxt->ctxt;
    int idx = c_ctxt->extra;
    return purc_variant_array_insert_before(array, idx, member);
}

static bool
insert_after_array_member(void* ctxt, purc_variant_t member,
        purc_variant_t member_extra, bool silently)
{
    UNUSED_PARAM(member_extra);
    UNUSED_PARAM(silently);
    struct complex_ctxt* c_ctxt = (struct complex_ctxt*) ctxt;
    purc_variant_t array = (purc_variant_t) c_ctxt->ctxt;
    int idx = c_ctxt->extra;
    return purc_variant_array_insert_after(array, idx, member);
}

static bool
add_set_member(void* ctxt, purc_variant_t member,
        purc_variant_t member_extra, bool silently)
{
    UNUSED_PARAM(member_extra);
    UNUSED_PARAM(silently);
    return purc_variant_set_add((purc_variant_t)ctxt, member, false);
}

static bool
remove_set_member(void* ctxt, purc_variant_t member,
        purc_variant_t member_extra, bool silently)
{
    UNUSED_PARAM(member_extra);
    UNUSED_PARAM(silently);
    return purc_variant_set_remove((purc_variant_t)ctxt, member);
}

static bool
add_set_member_overwrite(void* ctxt, purc_variant_t member,
        purc_variant_t member_extra, bool silently)
{
    UNUSED_PARAM(member_extra);
    UNUSED_PARAM(silently);
    return purc_variant_set_add((purc_variant_t)ctxt, member, true);
}

static bool
subtract_set(void* ctxt, purc_variant_t value,
        purc_variant_t member_extra, bool silently)
{
    UNUSED_PARAM(member_extra);
    UNUSED_PARAM(silently);
    struct complex_ctxt* c_ctxt = (struct complex_ctxt*) ctxt;
    purc_variant_t set = (purc_variant_t) c_ctxt->ctxt;
    purc_variant_t result = (purc_variant_t) c_ctxt->extra;

    return is_in_set(set, value) ? true :
        purc_variant_array_append(result, value);
}

static bool
subtract_array(void* ctxt, purc_variant_t value,
        purc_variant_t member_extra, bool silently)
{
    UNUSED_PARAM(member_extra);
    UNUSED_PARAM(silently);
    struct complex_ctxt* c_ctxt = (struct complex_ctxt*) ctxt;
    purc_variant_t array = (purc_variant_t) c_ctxt->ctxt;
    purc_variant_t result = (purc_variant_t) c_ctxt->extra;

    return is_in_array(array, value, NULL) ? true :
        purc_variant_array_append(result, value);
}

static bool
intersect_set(void* ctxt, purc_variant_t value,
        purc_variant_t member_extra, bool silently)
{
    UNUSED_PARAM(member_extra);
    UNUSED_PARAM(silently);
    struct complex_ctxt* c_ctxt = (struct complex_ctxt*) ctxt;
    purc_variant_t set = (purc_variant_t) c_ctxt->ctxt;
    purc_variant_t result = (purc_variant_t) c_ctxt->extra;

    return is_in_set(set, value) ? purc_variant_array_append(result, value) :
        true;
}

static bool
object_displace(purc_variant_t dst, purc_variant_t src, bool silently)
{
    bool ret = false;

    if (!purc_variant_is_object(src)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    if (!variant_object_clear(dst, silently)) {
        goto end;
    }

    if(!object_foreach(src, add_object_member, dst, silently)) {
        goto end;
    }
    ret = true;

end:
    return ret;
}

static bool
object_remove(purc_variant_t dst, purc_variant_t src, bool silently)
{
    bool ret = false;

    if (!purc_variant_is_object(src)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    if(!object_foreach(src, remove_object_member, dst, silently)) {
        goto end;
    }
    ret = true;

end:
    return ret;
}

static bool
array_displace(purc_variant_t dst, purc_variant_t src, bool silently)
{
    bool ret = false;

    if (!purc_variant_is_array(src) && !purc_variant_is_set(src)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    if (!variant_array_clear(dst, silently)) {
        goto end;
    }

    if (purc_variant_is_array(src)) {
        ret = array_foreach(src, append_array_member, dst, silently);
    }
    else {
        ret = set_foreach(src, append_array_member, dst, silently);
    }

end:
    return ret;
}

static bool
array_remove(purc_variant_t dst, purc_variant_t src, bool silently)
{
    bool ret = false;

    if (!purc_variant_is_array(src) && !purc_variant_is_set(src)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    if (purc_variant_is_array(src)) {
        ret = array_foreach(src, remove_array_member, dst, silently);
    }
    else {
        ret = set_foreach(src, remove_array_member, dst, silently);
    }

end:
    return ret;
    return false;
}

static bool
set_displace(purc_variant_t dst, purc_variant_t src, bool silently)
{
    bool ret = false;

    enum purc_variant_type type = purc_variant_get_type(src);
    switch (type) {
        case PURC_VARIANT_TYPE_OBJECT:
            if (!variant_set_clear(dst, silently)) {
                goto end;
            }
            if (!purc_variant_set_add(dst, src, false)) {
                goto end;
            }
            ret = true;
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            if (!variant_set_clear(dst, silently)) {
                goto end;
            }
            if (!array_foreach(src, add_set_member, dst, silently)) {
                goto end;
            }
            ret = true;
            break;

        case PURC_VARIANT_TYPE_SET:
            if (!variant_set_clear(dst, silently)) {
                goto end;
            }
            if (!set_foreach(src, add_set_member, dst, silently)) {
                goto end;
            }
            ret = true;
            break;

        default:
            SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
            ret = false;
            break;
    }

end:
    return ret;
}

static bool
set_remove(purc_variant_t dst, purc_variant_t src, bool silently)
{
    bool ret = false;

    enum purc_variant_type type = purc_variant_get_type(src);
    switch (type) {
        case PURC_VARIANT_TYPE_OBJECT:
            // TODO: silent
            if (!purc_variant_set_remove(dst, src)) {
                goto end;
            }
            ret = true;
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            if (!array_foreach(src, remove_set_member, dst, silently)) {
                goto end;
            }
            ret = true;
            break;

        case PURC_VARIANT_TYPE_SET:
            if (!set_foreach(src, remove_set_member, dst, silently)) {
                goto end;
            }
            ret = true;
            break;

        default:
            SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
            ret = false;
            break;
    }

end:
    return ret;
}

bool
purc_variant_container_displace(purc_variant_t dst,
        purc_variant_t src, bool silently)
{
    bool ret = false;

    if (dst == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (dst == src) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_OPERAND);
        goto end;
    }

    enum purc_variant_type type = purc_variant_get_type(dst);
    switch (type) {
        case PURC_VARIANT_TYPE_OBJECT:
            ret = object_displace(dst, src, silently);
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            ret = array_displace(dst, src, silently);
            break;

        case PURC_VARIANT_TYPE_SET:
            ret = set_displace(dst, src, silently);
            break;

        default:
            SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
            break;
    }

end:
    return ret;
}

bool
purc_variant_container_remove(purc_variant_t dst,
        purc_variant_t src, bool silently)
{
    bool ret = false;

    if (dst == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (dst == src) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_OPERAND);
        goto end;
    }

    enum purc_variant_type type = purc_variant_get_type(dst);
    switch (type) {
        case PURC_VARIANT_TYPE_OBJECT:
            ret = object_remove(dst, src, silently);
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            ret = array_remove(dst, src, silently);
            break;

        case PURC_VARIANT_TYPE_SET:
            ret = set_remove(dst, src, silently);
            break;

        default:
            SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
            break;
    }

end:
    return ret;
}

bool
purc_variant_array_append_another(purc_variant_t array,
        purc_variant_t another, bool silently)
{
    bool ret = false;

    if (array == PURC_VARIANT_INVALID || another == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (array == another) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_OPERAND);
        goto end;
    }

    if (!purc_variant_is_array(array) || !purc_variant_is_array(another)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    if(!array_foreach(another, append_array_member, array, silently)) {
        goto end;
    }
    ret = true;

end:
    return ret;
}

bool
purc_variant_array_prepend_another(purc_variant_t array,
        purc_variant_t another, bool silently)
{
    bool ret = false;

    if (array == PURC_VARIANT_INVALID || another == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (array == another) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_OPERAND);
        goto end;
    }

    if (!purc_variant_is_array(array) || !purc_variant_is_array(another)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    if(!array_reverse_foreach(another, prepend_array_member, array, silently)) {
        goto end;
    }
    ret = true;

end:
    return ret;
}

bool
purc_variant_object_merge_another(purc_variant_t object,
        purc_variant_t another, bool silently)
{
    bool ret = false;

    if (object == PURC_VARIANT_INVALID || another == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (object == another) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_OPERAND);
        goto end;
    }

    if (!purc_variant_is_object(object) || !purc_variant_is_object(another)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    //TODO : id conflict
    ret = object_foreach(another, add_object_member, object, silently);

end:
    return ret;
}

bool
purc_variant_array_insert_another_before(purc_variant_t array, int idx,
        purc_variant_t another, bool silently)
{
    bool ret = false;

    if (array == PURC_VARIANT_INVALID || another == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (array == another) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_OPERAND);
        goto end;
    }

    if (!purc_variant_is_array(array) || !purc_variant_is_array(another)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    struct complex_ctxt c_ctxt;
    c_ctxt.ctxt = (uintptr_t) array;
    c_ctxt.extra = idx;
    ret = array_reverse_foreach(another, insert_before_array_member, &c_ctxt,
            silently);

end:
    return ret;
}

bool
purc_variant_array_insert_another_after(purc_variant_t array, int idx,
        purc_variant_t another, bool silently)
{
    bool ret = false;

    if (array == PURC_VARIANT_INVALID || another == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (array == another) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_OPERAND);
        goto end;
    }

    if (!purc_variant_is_array(array) || !purc_variant_is_array(another)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    struct complex_ctxt c_ctxt;
    c_ctxt.ctxt = (uintptr_t) array;
    c_ctxt.extra = idx;
    ret = array_reverse_foreach(another, insert_after_array_member, &c_ctxt,
            silently);

end:
    return ret;
}

bool
purc_variant_set_unite(purc_variant_t set,
        purc_variant_t src, bool silently)
{
    bool ret = false;

    if (set == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (set == src) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_OPERAND);
        goto end;
    }

    if (!purc_variant_is_set(set)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    if (purc_variant_is_set(src)) {
        ret = set_foreach(src, add_set_member_overwrite, set, silently);
    }
    else if (purc_variant_is_array(src)) {
        ret = array_foreach(src, add_set_member_overwrite, set, silently);
    }
    else {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        ret = false;
    }

end:
    return ret;
}

bool
purc_variant_set_intersect(purc_variant_t set,
        purc_variant_t src, bool silently)
{
    bool ret = false;

    if (set == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (set == src) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_OPERAND);
        goto end;
    }

    if (!purc_variant_is_set(set)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    purc_variant_t result = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (result == PURC_VARIANT_INVALID) {
        goto end;
    }

    struct complex_ctxt c_ctxt;
    c_ctxt.ctxt = (uintptr_t) set;
    c_ctxt.extra = (uintptr_t) result;

    if (purc_variant_is_set(src)) {
        if(set_foreach(src, intersect_set, &c_ctxt, silently)) {
            ret = set_displace(set, result, silently);
        }
    }
    else if (purc_variant_is_array(src)) {
        if(array_foreach(src, intersect_set, &c_ctxt, silently)) {
            ret = set_displace(set, result, silently);
        }
    }
    else {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        ret = false;
    }

    purc_variant_unref(result);
end:
    return ret;
}

bool
purc_variant_set_subtract(purc_variant_t set,
        purc_variant_t src, bool silently)
{
    bool ret = false;

    if (set == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (set == src) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_OPERAND);
        goto end;
    }

    if (!purc_variant_is_set(set)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    purc_variant_t result = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (result == PURC_VARIANT_INVALID) {
        goto end;
    }

    struct complex_ctxt c_ctxt;
    c_ctxt.ctxt = (uintptr_t) src;
    c_ctxt.extra = (uintptr_t) result;

    if (purc_variant_is_set(src)) {
        if (set_foreach(set, subtract_set, &c_ctxt, silently)) {
            ret = set_displace(set, result, silently);
        }
    }
    else if (purc_variant_is_array(src)) {
        if (set_foreach(set, subtract_array, &c_ctxt, silently)) {
            ret = set_displace(set, result, silently);
        }
    }
    else {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        ret = false;
    }

    purc_variant_unref(result);
end:
    return ret;
}

bool
purc_variant_set_xor(purc_variant_t set,
        purc_variant_t src, bool silently)
{
    bool ret = false;

    if (set == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (set == src) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_OPERAND);
        goto end;
    }

    if (!purc_variant_is_set(set)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    purc_variant_t result = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (result == PURC_VARIANT_INVALID) {
        goto end;
    }

    struct complex_ctxt c_ctxt;
    if (purc_variant_is_set(src)) {
        c_ctxt.ctxt = (uintptr_t) src;
        c_ctxt.extra = (uintptr_t) result;
        if (!set_foreach(set, subtract_set, &c_ctxt, silently)) {
            goto error;
        }

        c_ctxt.ctxt = (uintptr_t) set;
        c_ctxt.extra = (uintptr_t) result;
        if (!set_foreach(src, subtract_set, &c_ctxt, silently)) {
            goto error;
        }
        ret = set_displace(set, result, silently);
    }
    else if (purc_variant_is_array(src)) {
        c_ctxt.ctxt = (uintptr_t) src;
        c_ctxt.extra = (uintptr_t) result;
        if (!set_foreach(set, subtract_array, &c_ctxt, silently)) {
            goto error;
        }

        c_ctxt.ctxt = (uintptr_t) set;
        c_ctxt.extra = (uintptr_t) result;
        if (!array_foreach(src, subtract_set, &c_ctxt, silently)) {
            goto error;
        }
        ret = set_displace(set, result, silently);
    }
    else {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        ret = false;
    }

error:
    purc_variant_unref(result);

end:
    return ret;
}

bool
purc_variant_set_overwrite(purc_variant_t set,
        purc_variant_t src, bool silently)
{
    bool ret = false;

    if (set == PURC_VARIANT_INVALID || src == PURC_VARIANT_INVALID) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_VALUE);
        goto end;
    }

    if (set == src) {
        SET_SILENT_ERROR(PURC_ERROR_INVALID_OPERAND);
        goto end;
    }

    if (!purc_variant_is_set(set)) {
        SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
        goto end;
    }

    enum purc_variant_type type = purc_variant_get_type(src);
    switch (type) {
        case PURC_VARIANT_TYPE_OBJECT:
            ret = purc_variant_set_add(set, src, true);
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            ret = array_foreach(src, add_set_member_overwrite, set, silently);
            break;

        case PURC_VARIANT_TYPE_SET:
            ret = set_foreach(src, add_set_member_overwrite, set, silently);
            break;

        default:
            SET_SILENT_ERROR(PURC_ERROR_WRONG_DATA_TYPE);
            break;
    }

end:
    return ret;
}

