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

#include "types.h"

ID3v2_tag* new_tag()
{
    ID3v2_tag* tag = malloc(sizeof(ID3v2_tag));
    tag->tag_header = new_header();
    tag->frames = new_frame_list();
    return tag;
}

ID3v2_header* new_header()
{
    ID3v2_header* tag_header = malloc(sizeof(ID3v2_header));
    if(tag_header != NULL)
    {
        memset(tag_header->tag, '\0', ID3_HEADER_TAG);
        tag_header->minor_version = 0x00;
        tag_header->major_version = 0x00;
        tag_header->flags = 0x00;
        memset(tag_header->tag, 0, ID3_HEADER_SIZE);
    }
    
    return tag_header;
}

ID3v2_frame* new_frame()
{
    ID3v2_frame* frame = malloc(sizeof(ID3v2_frame));
    return frame;
}

ID3v2_frame_list* new_frame_list()
{
    ID3v2_frame_list* list = malloc(sizeof(ID3v2_frame_list));
    if(list != NULL)
    {
        list->frame = NULL;
        list->next = NULL;
        list->start = NULL;
    }
    return list;
}

ID3v2_frame_text_content* new_text_content(int size)
{
    ID3v2_frame_text_content* content = malloc(sizeof(ID3v2_frame_text_content));
    content->data = malloc(size);
    return content;
}

void free_text_content(ID3v2_frame_text_content *content)
{
    if (!content) return;
    free(content->data);
    content->data = NULL;
    free(content);
}

ID3v2_frame_comment_content* new_comment_content(int size)
{
    ID3v2_frame_comment_content* content = malloc(sizeof(ID3v2_frame_comment_content));
    content->text = new_text_content(size - ID3_FRAME_SHORT_DESCRIPTION - ID3_FRAME_LANGUAGE);
    content->language = malloc(ID3_FRAME_LANGUAGE + sizeof(char));
    return content;
}

ID3v2_frame_apic_content* new_apic_content()
{
    ID3v2_frame_apic_content* content = malloc(sizeof(ID3v2_frame_apic_content));
    return content;
}

void free_apic_content(ID3v2_frame_apic_content *content)
{
    if (!content) return;
    free(content->data);
    content->data = NULL;
    free(content->mime_type);
    content->mime_type = NULL;
    free(content);
}
