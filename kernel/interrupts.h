#pragma once
#include <stdint.h>

typedef void (*irq_handler_t)(void);

void pic_remap(void);
void irq_install_handler(int irq, irq_handler_t h);
void pic_unmask(int irq);
void pic_mask(int irq);