#include "air.h"
#include <stdlib.h>
#include <string.h>

air_instr_list_t *air_instr_list_new()
{
    air_instr_list_t *list = (air_instr_list_t *)malloc(sizeof(*list));
    if (list) {
        list->head = NULL;
        list->tail = NULL;
        list->count = 0;
    }
    return list;
}

void air_instr_list_free(air_instr_list_t *list)
{
    air_instr_t *node = list->head;
    while (node) {
        air_instr_t *next = node->next;
        free(node);
        node = next;
    }
    list->head = NULL;
    list->tail = NULL;
}

void air_instr_list_add(air_instr_list_t *list, air_instr_t *instr)
{
    if (!list->head) {
        list->head = instr;
        list->tail = instr;
    }
    else {
        list->tail->next = instr;
        list->tail = instr;
    }
}
