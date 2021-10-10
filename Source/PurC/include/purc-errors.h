/**
 * @file purc-errors.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/02
 * @brief The error codes of PurC.
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

#ifndef PURC_PURC_ERRORS_H
#define PURC_PURC_ERRORS_H

#include <stdbool.h>
#include <stddef.h>

#include "purc-macros.h"

#define PURC_ERROR_OK                   0
#define PURC_ERROR_BAD_SYSTEM_CALL      1
#define PURC_ERROR_BAD_STDC_CALL        2
#define PURC_ERROR_OUT_OF_MEMORY        3
#define PURC_ERROR_INVALID_VALUE        4
#define PURC_ERROR_DUPLICATED           5
#define PURC_ERROR_NOT_IMPLEMENTED      6
#define PURC_ERROR_NO_INSTANCE          7
#define PURC_ERROR_TOO_LARGE_ENTITY     8
#define PURC_ERROR_BAD_ENCODING         9
#define PURC_ERROR_NOT_SUPPORTED        10
#define PURC_ERROR_OUTPUT               11
#define PURC_ERROR_TOO_SMALL_BUFF       12
#define PURC_ERROR_NULL_OBJECT          13
#define PURC_ERROR_TOO_SMALL_SIZE       14
#define PURC_ERROR_INCOMPLETE_OBJECT    15
#define PURC_ERROR_NO_FREE_SLOT         16
#define PURC_ERROR_NOT_EXISTS           17
#define PURC_ERROR_WRONG_ARGS           18
#define PURC_ERROR_WRONG_STAGE          19
#define PURC_ERROR_UNEXPECTED_RESULT    20
#define PURC_ERROR_UNEXPECTED_DATA      21
#define PURC_ERROR_OVERFLOW             22
#define PURC_ERROR_UNKNOWN              23

// the first error codes for various modules:
#define PURC_ERROR_FIRST_VARIANT        100
#define PURC_ERROR_FIRST_RWSTREAM       200

#define PURC_ERROR_FIRST_EJSON          1100
#define PURC_ERROR_FIRST_HVML           1200
#define PURC_ERROR_FIRST_HTML           1300
#define PURC_ERROR_FIRST_XGML           1400
#define PURC_ERROR_FIRST_XML            1500

#define PURC_ERROR_FIRST_VDOM           2100
#define PURC_ERROR_FIRST_EDOM           2200
#define PURC_ERROR_FIRST_VCM            2300

#define PURC_ERROR_FIRST_EXECUTOR       2400

// TODO: error codes for variant go here
enum pcvariant_error
{
    PCVARIANT_SUCCESS = PURC_ERROR_OK,
    PCVARIANT_INVALID_TYPE = PURC_ERROR_FIRST_VARIANT,
    PCVARIANT_STRING_NOT_UTF8,
    PCVARIANT_ERROR_NOT_FOUND,
};

// TODO: error codes for rwstream go here
enum pcrwstream_error
{
    PCRWSTREAM_SUCCESS = PURC_ERROR_OK,
    PCRWSTREAM_ERROR_FAILED = PURC_ERROR_FIRST_RWSTREAM,
    PCRWSTREAM_ERROR_FILE_TOO_BIG,
    PCRWSTREAM_ERROR_IO,
    PCRWSTREAM_ERROR_IS_DIR,
    PCRWSTREAM_ERROR_NO_SPACE,
    PCRWSTREAM_ERROR_NO_DEVICE_OR_ADDRESS,
    PCRWSTREAM_ERROR_OVERFLOW,
    PCRWSTREAM_ERROR_PIPE,
};

// TODO: error codes for ejson go here
enum pcejson_error
{
    PCEJSON_SUCCESS = PURC_ERROR_OK,
    PCEJSON_ERROR_UNEXPECTED_CHARACTER = PURC_ERROR_FIRST_EJSON,
    PCEJSON_ERROR_UNEXPECTED_NULL_CHARACTER,
    PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT,
    PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION,
    PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER,
    PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER,
    PCEJSON_ERROR_UNEXPECTED_RIGHT_BRACE,
    PCEJSON_ERROR_UNEXPECTED_RIGHT_BRACKET,
    PCEJSON_ERROR_UNEXPECTED_JSON_KEY_NAME,
    PCEJSON_ERROR_UNEXPECTED_COMMA,
    PCEJSON_ERROR_UNEXPECTED_JSON_KEYWORD,
    PCEJSON_ERROR_UNEXPECTED_BASE64,
    PCEJSON_ERROR_UNEXPECTED_EOF,
    PCEJSON_ERROR_BAD_JSON_NUMBER,
    PCEJSON_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY,
    PCEJSON_ERROR_BAD_JSON,
    PCEJSON_ERROR_MAX_DEPTH_EXCEEDED,
};

// TODO: error codes for hvml go here
enum pchvml_error
{
    PCHVML_SUCCESS = PURC_ERROR_OK,
    PCHVML_ERROR_UNEXPECTED_NULL_CHARACTER = PURC_ERROR_FIRST_HVML,
    PCHVML_ERROR_UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME,
    PCHVML_ERROR_EOF_BEFORE_TAG_NAME,
    PCHVML_ERROR_MISSING_END_TAG_NAME,
    PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME,
    PCHVML_ERROR_EOF_IN_TAG,
    PCHVML_ERROR_UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME,
    PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME,
    PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE,
    PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_ATTRIBUTES,
    PCHVML_ERROR_UNEXPECTED_SOLIDUS_IN_TAG,
    PCHVML_ERROR_CDATA_IN_HTML_CONTENT,
    PCHVML_ERROR_INCORRECTLY_OPENED_COMMENT,
    PCHVML_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT,
    PCHVML_ERROR_EOF_IN_COMMENT,
    PCHVML_ERROR_EOF_IN_DOCTYPE,
    PCHVML_ERROR_MISSING_WHITESPACE_BEFORE_DOCTYPE_NAME,
    PCHVML_ERROR_MISSING_DOCTYPE_NAME,
    PCHVML_ERROR_INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME,
    PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD,
    PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_IDENTIFIER,
    PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER,
    PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_IDENTIFIER,
    PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_INFORMATIONS,
    PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD,
    PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM_INFORMATION,
    PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM_INFORMATION,
    PCHVML_ERROR_UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM_INFORMATION,
    PCHVML_ERROR_EOF_IN_CDATA,
    PCHVML_ERROR_UNKNOWN_NAMED_CHARACTER_REFERENCE,
    PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE,
    PCHVML_ERROR_UNEXPECTED_CHARACTER,
    PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT,
    PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION,
    PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER,
    PCHVML_ERROR_UNEXPECTED_JSON_NUMBER,
    PCHVML_ERROR_UNEXPECTED_RIGHT_BRACE,
    PCHVML_ERROR_UNEXPECTED_RIGHT_BRACKET,
    PCHVML_ERROR_UNEXPECTED_JSON_KEY_NAME,
    PCHVML_ERROR_UNEXPECTED_COMMA,
    PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD,
    PCHVML_ERROR_UNEXPECTED_BASE64,
    PCHVML_ERROR_BAD_JSON_NUMBER,
    PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY,
    PCHVML_ERROR_BAD_JSONEE,
    PCHVML_ERROR_BAD_JSONEE_ESCAPE_ENTITY,
    PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME,
    PCHVML_ERROR_EMPTY_JSONEE_NAME,
    PCHVML_ERROR_BAD_JSONEE_NAME,
    PCHVML_ERROR_BAD_JSONEE_KEYWORD,
    PCHVML_ERROR_EMPTY_JSONEE_KEYWORD,
    PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_COMMA,
    PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_PARENTHESIS,
    PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_LEFT_ANGLE_BRACKET,
    PCHVML_ERROR_MISSING_MISSING_ATTRIBUTE_VALUE,
    PCHVML_ERROR_NESTED_COMMENT,
    PCHVML_ERROR_INCORRECTLY_CLOSED_COMMENT,
    PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_INFORMATION,
    PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE,
    PCHVML_ERROR_CHARACTER_REFERENCE_OUTSIDE_UNICODE_RANGE,
    PCHVML_ERROR_SURROGATE_CHARACTER_REFERENCE,
    PCHVML_ERROR_NONCHARACTER_CHARACTER_REFERENCE,
    PCHVML_ERROR_NULL_CHARACTER_REFERENCE,
    PCHVML_ERROR_CONTROL_CHARACTER_REFERENCE,
    PCHVML_ERROR_INVALID_UTF8_CHARACTER
};

enum pcexecutor_error
{
    PCEXECUTOR_SUCCESS = PURC_ERROR_OK,
    PCEXECUTOR_ERROR_OOM = PURC_ERROR_OUT_OF_MEMORY,
    PCEXECUTOR_ERROR_BAD_ARG = PURC_ERROR_INVALID_VALUE,
    PCEXECUTOR_ERROR_ALREAD_EXISTS = PURC_ERROR_DUPLICATED,
    PCEXECUTOR_ERROR_NOT_IMPLEMENTED = PURC_ERROR_FIRST_EXECUTOR,
};

PCA_EXTERN_C_BEGIN

/**
 * purc_get_last_error:
 *
 * Returns: The last error code.
 */
PCA_EXPORT int
purc_get_last_error (void);

/**
 * purc_get_error_message:
 *
 * @errcode: the error code.
 *
 * Returns: The message for the specified error code.
 */
PCA_EXPORT const char*
purc_get_error_message (int errcode);

PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_ERRORS_H */

