//
// Created by czy on 4/4/24.
//

#include <utils.h>
#include "isa.h"

int ring_buffer_index = -1;
char ring_buffer[MAX_INSTR_RING_BUFFER][128] = {};

void ring_buffer_display() {
    printf("----------------------------iringbuf----------------------------\n");
    for(int i = 0; i < MAX_INSTR_RING_BUFFER; ++i) {
        if (i == ring_buffer_index)
            printf("  -->  %s\n", ring_buffer[i]);
        else
            printf("       %s\n", ring_buffer[i]);
    }
    printf("----------------------------iringbuf----------------------------\n\n");
}

