/*
 * @file hvml-buffer.h
 * @author XueShuming
 * @date 2021/08/27
 * @brief The interfaces for hvml buffer.
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

#ifndef PURC_HVML_HVML_BUFFER_H
#define PURC_HVML_HVML_BUFFER_H

#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "purc-utils.h"

struct pchvml_buffer {
    uint8_t* base;
    uint8_t* here;
    uint8_t* stop;
    size_t nr_chars;
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pchvml_buffer* pchvml_buffer_new (void);

PCA_INLINE
bool pchvml_buffer_is_empty (struct pchvml_buffer* buffer)
{
    return buffer->here == buffer->base;
}

PCA_INLINE
size_t pchvml_buffer_get_size_in_bytes (struct pchvml_buffer* buffer)
{
    return buffer->here - buffer->base;
}

PCA_INLINE
size_t pchvml_buffer_get_size_in_chars (struct pchvml_buffer* buffer)
{
    return buffer->nr_chars;
}

PCA_INLINE
const char* pchvml_buffer_get_buffer (
        struct pchvml_buffer* buffer)
{
    return (const char*)buffer->base;
}

void pchvml_buffer_append_bytes (struct pchvml_buffer* buffer,
        const char* bytes, size_t nr_bytes);

void pchvml_buffer_append (struct pchvml_buffer* buffer,
        uint32_t uc);

void pchvml_buffer_append_chars (struct pchvml_buffer* buffer,
        const uint32_t* ucs, size_t nr_ucs);

PCA_INLINE
void pchvml_buffer_append_another(struct pchvml_buffer* buffer,
        struct pchvml_buffer* another)
{
    pchvml_buffer_append_bytes(buffer,
        pchvml_buffer_get_buffer(another),
        pchvml_buffer_get_size_in_bytes(another));
}

/*
 * delete characters from head
 */
void pchvml_buffer_delete_head_chars (
        struct pchvml_buffer* buffer, size_t sz);

/*
 * delete characters from tail
 */
void pchvml_buffer_delete_tail_chars (
        struct pchvml_buffer* buffer, size_t sz);

bool pchvml_buffer_end_with (struct pchvml_buffer* buffer,
        const char* bytes, size_t nr_bytes);

bool pchvml_buffer_equal_to (struct pchvml_buffer* buffer,
        const char* bytes, size_t nr_bytes);

uint32_t pchvml_buffer_get_last_char (struct pchvml_buffer* buffer);

void pchvml_buffer_reset (struct pchvml_buffer* buffer);

void pchvml_buffer_destroy (struct pchvml_buffer* buffer);

bool pchvml_buffer_is_int (struct pchvml_buffer* buffer);

bool pchvml_buffer_is_number(struct pchvml_buffer* buffer);

bool pchvml_buffer_is_whitespace(struct pchvml_buffer* buffer);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_HVML_HVML_BUFFER_H */

