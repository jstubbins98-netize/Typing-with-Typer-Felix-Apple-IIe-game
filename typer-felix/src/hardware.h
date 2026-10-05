#ifndef FELIX_HARDWARE_H
#define FELIX_HARDWARE_H

#define IO(addr) (*(volatile unsigned char *)(addr))
#define KEY_ESC 27
#define KEY_TAB 9
#define KEY_SOUND 19

extern unsigned char sound_on;
extern unsigned char refresh_hz;
void wait_frame(void);
void __fastcall__ tone(unsigned char period);
unsigned char key_read(void);
void key_flush(void);
unsigned char key_held(void);
void key_wait_release(void);
void sound_key(void);
void sound_error(void);
void sound_win(void);
void sound_miss(void);
void delay_frames(unsigned char n);

#endif