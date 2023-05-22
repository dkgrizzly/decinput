#ifndef __MOUSE_H
#define __MOUSE_H

typedef struct mouse_packet_s {
    int8_t x;
    int8_t y;

    uint8_t buttons;
} mouse_packet_t;

uint8_t mp_wrptr;
uint8_t mp_rdptr;

typedef struct mouse_status_s {
    bool selftest_done;

    uint8_t buttons;

    uint8_t laststate;

    int16_t dx;
    int16_t dy;
    
    uint16_t baud;
    uint8_t mode;

    struct repeating_timer timer;
} mouse_status_t;

extern void mouse_init();
extern void mouse_dowork();
extern void mouse_packet_enqueue(int8_t x, int8_t y, uint8_t buttons);

#endif /* __MOUSE_H */