/*
 * @file tokenizer.c
 * @author Xue Shuming
 * @date 2022/02/08
 * @brief The implementation of hvml parser.
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
 *
 */

#include "config.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dom.h"
#include "private/hvml.h"

#include "hvml-buffer.h"
#include "hvml-rwswrap.h"
#include "hvml-token.h"
#include "hvml-sbst.h"
#include "hvml-attr.h"
#include "hvml-tag.h"

#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#include "hvml_err_msgs.inc"

#if HAVE(GLIB)
#define    PCHVML_ALLOC(sz)   g_slice_alloc0(sz)
#define    PCHVML_FREE(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    PCHVML_ALLOC(sz)   calloc(1, sz)
#define    PCHVML_FREE(p)     free(p)
#endif

/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(msgs,
        PCA_TABLESIZE(hvml_err_msgs) == PCHVML_ERROR_NR);

#undef _COMPILE_TIME_ASSERT

static struct err_msg_seg _hvml_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_HVML,
    PURC_ERROR_FIRST_HVML + PCA_TABLESIZE(hvml_err_msgs) - 1,
    hvml_err_msgs
};

void pchvml_init_once(void)
{
    pcinst_register_error_message_segment(&_hvml_err_msgs_seg);
}

#define PRINT_LOG_SWITCH_FILE "/tmp/purc_hvml_tokenizer"

struct pchvml_parser* pchvml_create(uint32_t flags, size_t queue_size)
{
    UNUSED_PARAM(flags);
    UNUSED_PARAM(queue_size);

    struct pchvml_parser* parser = (struct pchvml_parser*) PCHVML_ALLOC(
            sizeof(struct pchvml_parser));
    parser->state = 0;
    parser->rwswrap = pchvml_rwswrap_new ();
    parser->temp_buffer = pchvml_buffer_new ();
    parser->tag_name = pchvml_buffer_new ();
    parser->string_buffer = pchvml_buffer_new ();
    parser->vcm_stack = pcvcm_stack_new();
    parser->ejson_stack = pcutils_stack_new(0);
    parser->char_ref_code = 0;
    parser->prev_separator = 0;
    parser->nr_quoted = 0;
    parser->tag_is_operation = false;
    parser->tag_has_raw_attr = false;
    struct stat st;
    parser->enable_print_log = (stat(PRINT_LOG_SWITCH_FILE, &st) == 0);

    return parser;
}

void pchvml_reset(struct pchvml_parser* parser, uint32_t flags,
        size_t queue_size)
{
    UNUSED_PARAM(flags);
    UNUSED_PARAM(queue_size);

    parser->state = 0;
    pchvml_rwswrap_destroy (parser->rwswrap);
    parser->rwswrap = pchvml_rwswrap_new ();
    pchvml_buffer_reset (parser->temp_buffer);
    pchvml_buffer_reset (parser->tag_name);
    pchvml_buffer_reset (parser->string_buffer);

    struct pcvcm_node* n = parser->vcm_node;
    parser->vcm_node = NULL;
    while (!pcvcm_stack_is_empty(parser->vcm_stack)) {
        struct pcvcm_node* node = pcvcm_stack_pop(parser->vcm_stack);
        pctree_node_append_child(
                (struct pctree_node*)node, (struct pctree_node*)n);
        n = node;
    }
    pcvcm_node_destroy(n);
    pcvcm_stack_destroy(parser->vcm_stack);
    parser->vcm_stack = pcvcm_stack_new();
    pcutils_stack_destroy(parser->ejson_stack);
    parser->ejson_stack = pcutils_stack_new(0);
    if (parser->token) {
        pchvml_token_destroy(parser->token);
        parser->token = NULL;
    }
    parser->char_ref_code = 0;
    parser->prev_separator = 0;
    parser->nr_quoted = 0;
    parser->tag_is_operation = false;
    parser->tag_has_raw_attr = false;
}

void pchvml_destroy(struct pchvml_parser* parser)
{
    if (parser) {
        pchvml_rwswrap_destroy (parser->rwswrap);
        pchvml_buffer_destroy (parser->temp_buffer);
        pchvml_buffer_destroy (parser->tag_name);
        pchvml_buffer_destroy (parser->string_buffer);
        if (parser->sbst) {
            pchvml_sbst_destroy(parser->sbst);
        }
        struct pcvcm_node* n = parser->vcm_node;
        parser->vcm_node = NULL;
        while (!pcvcm_stack_is_empty(parser->vcm_stack)) {
            struct pcvcm_node* node = pcvcm_stack_pop(parser->vcm_stack);
            pctree_node_append_child(
                    (struct pctree_node*)node, (struct pctree_node*)n);
            n = node;
        }
        pcvcm_node_destroy(n);
        pcvcm_stack_destroy(parser->vcm_stack);
        pcutils_stack_destroy(parser->ejson_stack);
        if (parser->token) {
            pchvml_token_destroy(parser->token);
        }
        PCHVML_FREE(parser);
    }
}

