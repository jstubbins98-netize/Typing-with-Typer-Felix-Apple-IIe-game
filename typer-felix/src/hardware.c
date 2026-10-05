#include "hardware.h"

unsigned char sound_on = 1;
unsigned char refresh_hz = 60;

/* Keyboard latch reads live in timing.s: cc65 can omit a discarded
 * volatile expression, so IO(0xC010); is NOT a reliable latch reset. */
void key_wait_release(void)
{
    unsigned char quiet = 0;
    while (quiet < 2) {
        wait_frame();
        if (key_held()) quiet = 0;
        else ++quiet;
    }
    key_flush();
}

void sound_key(void)
{
    if (sound_on) __asm__("bit $C030");
}

void sound_error(void)
{
    if (sound_on) tone(95);
}

void sound_win(void)
{
    unsigned char p;
    if (!sound_on) return;
    for (p = 65; p > 20; p -= 5) tone(p);
}

void sound_miss(void)
{
    if (sound_on) { tone(65); tone(90); }
}

void delay_frames(unsigned char n)
{
    while (n--) wait_frame();
}