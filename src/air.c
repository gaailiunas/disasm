#include "air.h"
#include <stdlib.h>
#include <string.h>

void air_instr_list_init(air_instr_list_t *list)
{
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    list->used_in_tail = 0;
}

air_instr_list_t *air_instr_list_new()
{
    air_instr_list_t *list = (air_instr_list_t *)malloc(sizeof(*list));
    if (list)
        air_instr_list_init(list);
    return list;
}

void air_instr_list_destroy(air_instr_list_t *list)
{
    air_instr_chunk_t *chunk = list->head;
    while (chunk) {
        air_instr_chunk_t *next = chunk->next;
        free(chunk);
        chunk = next;
    }
}

void air_instr_list_free(air_instr_list_t *list)
{
    air_instr_list_destroy(list);
    free(list);
}

air_instr_t *air_instr_list_get_new(air_instr_list_t *list)
{
    if (!list->tail || list->used_in_tail == AIR_CHUNK_CAPACITY) {
        air_instr_chunk_t *new_chunk =
            (air_instr_chunk_t *)malloc(sizeof(*new_chunk));
        if (!new_chunk) {
            return NULL;
        }
        new_chunk->next = NULL;
        if (!list->head) {
            list->head = new_chunk;
        }
        else {
            list->tail->next = new_chunk;
        }
        list->tail = new_chunk;
        list->used_in_tail = 0;
    }

    list->count++;
    return &list->tail->items[list->used_in_tail++];
}
