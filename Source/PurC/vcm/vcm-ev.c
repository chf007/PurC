/*
 * @file vcm-ev.c
 * @author XueShuming
 * @date 2021/09/02
 * @brief The impl of vcm expression variable
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "purc-utils.h"
#include "purc-errors.h"
#include "purc-rwstream.h"

#include "private/errors.h"
#include "private/stack.h"
#include "private/interpreter.h"
#include "private/utils.h"
#include "private/vcm.h"

#include "eval.h"

// expression variable
struct pcvcm_ev {
    struct pcvcm_node *vcm;
    purc_variant_t const_value;
    purc_variant_t last_value;
    bool release_vcm;
};

static purc_variant_t
eval_getter(void *native_entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    struct pcintr_stack *stack = pcintr_get_stack();
    if (!stack) {
        return PURC_VARIANT_INVALID;
    }
    return pcvcm_eval(vcm_ev->vcm, stack,
            (call_flags & PCVRT_CALL_FLAG_SILENTLY));
}

static purc_variant_t
eval_const_getter(void *native_entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    if (vcm_ev->const_value) {
        return vcm_ev->const_value;
    }

    vcm_ev->const_value = eval_getter(native_entity, nr_args, argv,
            (call_flags & PCVRT_CALL_FLAG_SILENTLY));
    return vcm_ev->const_value;
}

static purc_variant_t
vcm_ev_getter(void *native_entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    return purc_variant_make_boolean(true);
}


static purc_variant_t
last_value_getter(void *native_entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    return vcm_ev->last_value;
}

static purc_variant_t
last_value_setter(void *native_entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    if (nr_args == 0) {
        return PURC_VARIANT_INVALID;
    }

    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    if (vcm_ev->last_value) {
        purc_variant_unref(vcm_ev->last_value);
    }
    vcm_ev->last_value = argv[0];
    if (vcm_ev->last_value) {
        purc_variant_ref(vcm_ev->last_value);
    }
    return vcm_ev->last_value;
}

static inline purc_nvariant_method
property_getter(const char *key_name)
{
    if (strcmp(key_name, PCVCM_EV_PROPERTY_EVAL) == 0) {
        return eval_getter;
    }
    else if (strcmp(key_name, PCVCM_EV_PROPERTY_EVAL_CONST) == 0) {
        return eval_const_getter;
    }
    else if (strcmp(key_name, PCVCM_EV_PROPERTY_VCM_EV) == 0) {
        return vcm_ev_getter;
    }
    else if (strcmp(key_name, PCVCM_EV_PROPERTY_LAST_VALUE) == 0) {
        return last_value_getter;
    }

    return NULL;
}

static inline purc_nvariant_method
property_setter(const char *key_name)
{
    if (strcmp(key_name, PCVCM_EV_PROPERTY_LAST_VALUE) == 0) {
        return last_value_setter;
    }

    return NULL;
}

bool
on_observe(void *native_entity, const char *event_name,
        const char *event_subname)
{
    UNUSED_PARAM(event_name);
    UNUSED_PARAM(event_subname);
    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    struct pcintr_stack *stack = pcintr_get_stack();
    if (!stack) {
        return false;
    }
    vcm_ev->last_value = pcvcm_eval(vcm_ev->vcm, stack, false);
    return (vcm_ev->last_value) ? true : false;
}

static void
on_release(void *native_entity)
{
    struct pcvcm_ev *vcm_variant = (struct pcvcm_ev*)native_entity;
    if (vcm_variant->release_vcm) {
        free(vcm_variant->vcm);
    }
    if (vcm_variant->const_value) {
        purc_variant_unref(vcm_variant->const_value);
    }
    if (vcm_variant->last_value) {
        purc_variant_unref(vcm_variant->last_value);
    }
    free(vcm_variant);
}

purc_variant_t
pcvcm_to_expression_variable(struct pcvcm_node *vcm, bool release_vcm)
{
    static struct purc_native_ops ops = {
        .property_getter        = property_getter,
        .property_setter        = property_setter,
        .property_cleaner       = NULL,
        .property_eraser        = NULL,

        .updater                = NULL,
        .cleaner                = NULL,
        .eraser                 = NULL,

        .on_observe            = on_observe,
        .on_release            = on_release,
    };

    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)calloc(1,
            sizeof(struct pcvcm_ev));
    if (!vcm_ev) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = purc_variant_make_native(vcm_ev, &ops);
    if (v == PURC_VARIANT_INVALID) {
        free(vcm_ev);
        return PURC_VARIANT_INVALID;
    }

    vcm_ev->vcm = vcm;
    vcm_ev->release_vcm = release_vcm;

    return v;
}

