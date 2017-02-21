/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "frame.h"
#include "utils.h"
#include "constants.h"

static inline int is_valid_frame_id_char(char c) {
    if (c >= 'A' && c <= 'Z') return 1;
    if (c >= '0' && c <= '9') return 1;
    return 0;
}

ID3v2_frame *parse_frame(char *bytes, int offset, int version)
{
    // Validate that this looks like a real frame.  The spec says
    // "The frame ID [is] made out of the characters capital A-Z and 0-9"
    // (will also catch if we're into the padding)
    const char *f = bytes + offset;
    if (!is_valid_frame_id_char(f[0]) || !is_valid_frame_id_char(f[1]) ||
        !is_valid_frame_id_char(f[2]) || !is_valid_frame_id_char(f[3])) {
        return NULL;
    }

    ID3v2_frame *frame = new_frame();

    // Parse frame header
    memcpy(frame->frame_id, bytes + offset, ID3_FRAME_ID);

    frame->size = btoi(bytes, 4, offset += ID3_FRAME_ID);
    if (version == ID3v24) {
        frame->size = syncint_decode(frame->size);
    }

    memcpy(frame->flags, bytes + (offset += ID3_FRAME_SIZE), 2);

    // Load frame data
    frame->data = malloc(frame->size);
    memcpy(frame->data, bytes + (offset += ID3_FRAME_FLAGS), frame->size);

    return frame;
}

static inline int bytes_per_char_for_encoding(int encoding) {
    if (encoding == ID3_TEXT_ENCODING_UTF16_WITH_BOM ||
        encoding == ID3_TEXT_ENCODING_UTF16BE_WITHOUT_BOM) {
        return 2;
    } else {
        return 1;
    }
}

ID3v2_frame_text_content *parse_text_frame_content(ID3v2_frame *frame)
{
    ID3v2_frame_text_content *content;
    if (!frame) return NULL;

    if (frame->size < ID3_FRAME_ENCODING + 1) return NULL;	// Need at least 1 byte for encoding

    content = new_text_content();

    content->encoding = frame->data[0];
    content->size = frame->size - ID3_FRAME_ENCODING;

    // Before we load the frame data, make sure we have 2 x '\0' terminators (this is so that
    // library clients can assume ID3v2.4-type lists for all text fields - i.e. that there is
    // a terminating NUL character on every string in a text field, with a second NUL character
    // at the end of the (possible) list of text fields).

    char *text = frame->data + ID3_FRAME_ENCODING;
    int text_size = content->size;		// Number of bytes to copy from the frame

    int bytes_per_char = bytes_per_char_for_encoding(content->encoding);
    int penultimate_char = 1;
    if (text_size >= 2 * bytes_per_char + 1) {	// We have at least 2 chars, so safe to look at penultimate one
        if (bytes_per_char == 2) {
            uint16_t *wp = (uint16_t *)text;	// XXX won't work on strict alignment machines
            int ws = text_size / 2;
            penultimate_char = wp[ws - 2];
        } else {
            penultimate_char = text[text_size - 2];
        }
    }

    if (penultimate_char) content->size += bytes_per_char;	// We'll need at least one more terminating NUL char

    int last_char = 1;
    if (text_size >= 1 * bytes_per_char + 1) {	// We have at least 1 char, so safe to look at it
        if (bytes_per_char == 2) {
            uint16_t *wp = (uint16_t *)text;	// XXX won't work on strict alignment machines
            int ws = text_size / 2;
            last_char = wp[ws - 1];
        } else {
            last_char = text[text_size - 1];
        }
    }

    if (last_char) content->size += bytes_per_char;		// We'll need another terminating NUL char

    content->data = calloc(1, content->size);

    memcpy(content->data, text, text_size);

    return content;
}

ID3v2_frame_comment_content *parse_comment_frame_content(ID3v2_frame *frame)
{
    ID3v2_frame_comment_content *content;
    if (!frame) return NULL;

    content = new_comment_content(frame->size);

    content->text->encoding = frame->data[0];
    content->text->size = frame->size - ID3_FRAME_ENCODING - ID3_FRAME_LANGUAGE - ID3_FRAME_SHORT_DESCRIPTION;
    memcpy(content->language, frame->data + ID3_FRAME_ENCODING, ID3_FRAME_LANGUAGE);
    content->short_description = "\0"; // Ignore short description
    memcpy(content->text->data, frame->data + ID3_FRAME_ENCODING + ID3_FRAME_LANGUAGE + 1, content->text->size);

    return content;
}

// Includes terminating NUL char in returned length
static char *parse_mime_type(char *data, int *len)
{
    char *mime_type = strdup(data);
    *len = (int)strlen(mime_type) + 1;

    return mime_type;
}

ID3v2_frame_apic_content *parse_apic_frame_content(ID3v2_frame *frame)
{
    if (!frame) return NULL;

    ID3v2_frame_apic_content *content = new_apic_content();

    int pos = 0;
    content->encoding = frame->data[pos++];

    int mime_type_len;	// (will include terminating NUL character)
    content->mime_type = parse_mime_type(frame->data + pos, &mime_type_len);
    pos += mime_type_len;

    content->picture_type = frame->data[pos++];
    content->description = &frame->data[pos];	// DO NOT free() this

    if (bytes_per_char_for_encoding(content->encoding) == 2) {
        /* skip UTF-16 description */
        while (*(uint16_t *)(frame->data + pos)) pos += 2;
        pos += 2;
    } else {
        /* skip UTF-8 or Latin-1 description */
        while (frame->data[pos] != '\0') pos++;
        pos++;
    }

    content->picture_size = frame->size - pos;
    content->data = malloc(content->picture_size);
    memcpy(content->data, frame->data + pos, content->picture_size);

    return content;
}
