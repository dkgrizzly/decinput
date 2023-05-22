#ifndef __KEYBOARD_H
#define __KEYBOARD_H

extern bool keystate[256];
extern bool ledstate[4];

extern void keyboard_init();
extern void keyboard_dowork();
extern void keyboard_sound(uint ms);

#endif /* __KEYBOARD_H */