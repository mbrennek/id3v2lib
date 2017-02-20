/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "header.h"
#include "utils.h"


static inline int has_id3v2tag(const char *raw_header)
{
    return (memcmp(raw_header, "ID3", 3) == 0);
}

ID3v2_header *get_tag_header(const char *file_name)
{
    char buffer[ID3_HEADER];
    FILE *file = fopen(file_name, "rb");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    fread(buffer, ID3_HEADER, 1, file);
    fclose(file);
    return get_tag_header_with_buffer(buffer, ID3_HEADER);
}

ID3v2_header *get_tag_header_with_buffer(const char *buffer, int length)
{
    int position = 0;
    ID3v2_header *tag_header;

    if (length < ID3_HEADER) return NULL;
    if (!has_id3v2tag(buffer)) return NULL;

    tag_header = new_header();

    memcpy(tag_header->tag, buffer, ID3_HEADER_TAG);
    tag_header->major_version = buffer[position += ID3_HEADER_TAG];
    tag_header->minor_version = buffer[position += ID3_HEADER_VERSION];
    tag_header->flags = buffer[position += ID3_HEADER_REVISION];
    tag_header->tag_size = syncint_decode(btoi(buffer, ID3_HEADER_SIZE, position += ID3_HEADER_FLAGS));

    if (tag_header->flags & ID3_HEADER_FLAGS_HAS_UNSYNCHRONISATION) {
        tag_header->unsynchronised = 1;
    }

    if (tag_header->flags & ID3_HEADER_FLAGS_HAS_EXTENDED_HEADER) {
        // an extended header exists, so we retrieve the actual size of it and save it into the struct
        tag_header->extended_header_size = syncint_decode(btoi(buffer, ID3_EXTENDED_HEADER_SIZE, position += ID3_HEADER_SIZE));
    } else {
        // no extended header existing
        tag_header->extended_header_size = 0;
    }

    return tag_header;
}

int get_tag_version(ID3v2_header *tag_header)
{
    switch (tag_header->major_version) {
        case 3:
            return ID3v23;
        case 4:
            return ID3v24;
        default:
            return NO_COMPATIBLE_TAG;
    }
}
