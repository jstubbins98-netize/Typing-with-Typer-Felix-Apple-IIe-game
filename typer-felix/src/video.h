#ifndef FELIX_VIDEO_H
#define FELIX_VIDEO_H

#define POSE_IDLE 0
#define POSE_POUNCE 1
#define POSE_TYPO 2
#define POSE_SLEEP 3
#define POSE_TYPE 4

void video_init(void);
void text_mode(unsigned char wide);
void text_clear(void);
void text_line(unsigned char y, const char *s);
void text_at(unsigned char x, unsigned char y, const char *s);
void text_char(unsigned char x, unsigned char y, unsigned char c,
               unsigned char inverse);
void graphics_mode(unsigned char mice);
void cat_show(unsigned char pose);
void target_make(const char *label, unsigned char mouse);
void target_show(unsigned char x, unsigned char y);
void target_hide(void);
void graphic_label(unsigned char column, unsigned char y, const char *s);

#endif