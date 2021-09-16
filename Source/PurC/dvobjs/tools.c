/*
 * @file tools.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of tools for all files in this directory.
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

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/html.h"

#include "purc-variant.h"

#include "tools.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>
#include <sys/time.h>

const char* pcdvobjs_get_next_option (const char* data, const char* delims, 
                                                            size_t* length)
{
    const char* head = data;
    char* temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00))
        return NULL;

    *length = 0;

    while (*data != 0x00) {
        temp = strchr (delims, *data);
        if (temp) {
            if (head == data) {
                head = data + 1;
            }
            else 
                break;
        }
        data++;
    }

    *length = data - head;
    if (*length == 0)
        head = NULL;

    return head;
}

const char* pcdvobjs_get_prev_option (const char* data, size_t str_len, 
                            const char* delims, size_t* length)
{
    const char* head = NULL;
    size_t tail = *length;
    char* temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00) ||
                                                        (str_len == 0))
        return NULL;

    *length = 0;

    while (str_len) {
        temp = strchr (delims, *(data + str_len - 1));
        if (temp) {
            if (tail == str_len) {
                str_len--;
                tail = str_len;
            }
            else 
                break;
        }
        str_len--;
    }

    *length = tail - str_len;
    if (*length == 0)
        head = NULL;
    else
        head = data + str_len;

    return head;
}

// for file to get '\n'
const char* pcdvobjs_file_get_next_option (const char* data, 
                                  const char* delims, size_t* length)
{
    const char* head = data;
    char* temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00))
        return NULL;

    *length = 0;

    while (*data != 0x00) {
        temp = strchr (delims, *data);
        if (temp) 
            break;
        data++;
    }

    *length = data - head;

    return head;
}

const char* pcdvobjs_file_get_prev_option (const char* data, size_t str_len, 
                            const char* delims, size_t* length)
{
    const char* head = NULL;
    size_t tail = str_len;
    char* temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00) ||
                                                        (str_len == 0))
        return NULL;

    *length = 0;

    while (str_len) {
        temp = strchr (delims, *(data + str_len - 1));
        if (temp) 
            break;
        str_len--;
    }

    *length = tail - str_len;
    head = data + str_len;

    return head;
}

const char * pcdvobjs_remove_space (char * buffer)
{
    int i = 0;
    int j = 0;
    while (*(buffer + i) != 0x00) {
        if (*(buffer + i) != ' ') {
            *(buffer + j) = *(buffer + i);
            j++;
        }
        i++;
    }
    *(buffer + j) = 0x00;

    return buffer;
}

bool wildcard_cmp (const char *str1, const char *pattern)
{
    if (str1 == NULL) 
        return false;
    if (pattern == NULL) 
        return false;

    int len1 = strlen (str1);
    int len2 = strlen (pattern);
    int mark = 0;
    int p1 = 0;
    int p2 = 0;

    while ((p1 < len1) && (p2<len2))
    {
        if (pattern[p2] == '?')
        {
            p1++;
            p2++;
            continue;
        }
        if (pattern[p2] == '*')
        {
            p2++;
            mark = p2;
            continue;
        }
        if (str1[p1] != pattern[p2])
        {
            if (p1 == 0 && p2 == 0)
                return false;
            p1 -= p2 - mark - 1;
            p2 = mark;
            continue;
        }
        p1++;
        p2++;
    }
    if (p2 == len2)
    {
        if (p1 == len1)
            return true;
        if (pattern[p2 - 1] == '*')
            return true;
    }
    while (p2 < len2)
    {
        if (pattern[p2] != '*')
            return false;
        p2++;
    }
    return true;
}

int pcdvobjs_math_param_set_var(struct pcdvobjs_math_param *param,
        const char *var, struct pcdvobjs_math_value *val)
{
    if (!param->variables) {
        param->variables = purc_variant_make_object(0, NULL, NULL);
        if (!param->variables) {
            return -1;
        }
    }
    purc_variant_t v;
    if (param->is_long_double) {
        v = purc_variant_make_longdouble(val->ld);
    } else {
        v = purc_variant_make_number(val->d);
    }
    if (!v)
        return -1;

    purc_variant_t k = purc_variant_make_string(var, true);
    bool ok;
    ok = purc_variant_object_set(param->variables, k, v);
    purc_variant_unref(k);
    purc_variant_unref(v);

    return ok ? 0 : -1;
}

int pcdvobjs_math_param_get_var(struct pcdvobjs_math_param *param,
        const char *var, struct pcdvobjs_math_value *val)
{
    if (!param->variables) {
        return -1;
    }
    purc_variant_t v;
    v = purc_variant_object_get_by_ckey(param->variables, var);
    if (!v)
        return -1;

    bool ok;
    if (param->is_long_double) {
        ok = purc_variant_cast_to_long_double(v, &val->ld, false);
    } else {
        ok = purc_variant_cast_to_number(v, &val->d, false);
    }

    return ok ? 0 : -1;
}

