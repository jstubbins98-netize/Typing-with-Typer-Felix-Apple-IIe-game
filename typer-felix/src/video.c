#include <string.h>
#include "hardware.h"
#include "video.h"

#define SW(addr) (IO(addr) = 0)
#define SPRITE_W 8
#define SPRITE_H 26

typedef struct {
    unsigned char bits[SPRITE_H][SPRITE_W];
    unsigned char mask[SPRITE_H][SPRITE_W];
    unsigned char saved[SPRITE_H][SPRITE_W];
    unsigned char x, y, visible;
} Sprite;

static unsigned char *rows[160];
static unsigned char wide_mode;
static Sprite cat;
static Sprite target;

/* 5x7 uppercase alphabet, then digits and semicolon. Bit 4 is leftmost. */
static const unsigned char font[][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14}, {7,2,2,2,18,18,12},
    {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14},
    {0,4,4,0,4,4,8}
};

static const char * const cat_art[] = {
    "    OO          OO          ",
    "    OOO        OOO          ",
    "    OXOO      OOXO          ",
    "    OOXOOOOOOOOXOO          ",
    "    OOOOOXXOOOOOOO          ",
    "   OOOOOOXXOOOOOOOO         ",
    "   OOOWWOOOOOOWWOOO         ",
    "   OOOWXOOOOOOXWOOO         ",
    "   OOOOOOOXXOOOOOOO         ",
    "  WWWOOOOOXXOOOOOWWW        ",
    "   OOOOOWOOOOOWOOOO         ",
    "    OOOOOOWWOOOOOO          ",
    "      OOOOOOOOOO            ",
    "     OOOOOOOOOOOO           ",
    "    OOOXXOOOOXXOOO       OO ",
    "    OOOXXOOOOXXOOO      OOO ",
    "    OOOOOOOOOOOOOO      OO  ",
    "    OOOXXOOOOXXOOOO    OOO  ",
    "    OOOXXOOOOXXOOOOOOOOOO   ",
    "    OOOOOOOOOOOOOOOOOOOO    ",
    "     OOOOOOOOOOOOOOOOO      ",
    "     OOOO    OOOO           ",
    "    OOOOOO  OOOOOO          ",
    "    OOOOOO  OOOOOO          "
};

static unsigned int text_address(unsigned char y)
{
    return 0x400U + ((unsigned int)(y & 7) << 7) + (y >> 3) * 40U;
}

void text_char(unsigned char x, unsigned char y, unsigned char c,
               unsigned char inverse)
{
    unsigned char *p;
    if (y >= 24 || x >= (wide_mode ? 80 : 40)) return;
    p = (unsigned char *)(text_address(y) + (wide_mode ? x >> 1 : x));
    if (wide_mode) {
        /* 80STORE maps only $0400-$07FF, not the C stack or code. */
        if (x & 1) SW(0xC054); else SW(0xC055);
    }
    if (inverse) {
        if (c >= 'a' && c <= 'z') c -= 32;
        *p = c & 0x3F;
    } else *p = c | 0x80;
    if (wide_mode) SW(0xC054);
}

void text_at(unsigned char x, unsigned char y, const char *s)
{
    unsigned char limit = wide_mode ? 80 : 40;
    while (*s && x < limit) text_char(x++, y, *s++, 0);
}

void text_line(unsigned char y, const char *s)
{
    unsigned char x, n = wide_mode ? 80 : 40;
    for (x = 0; x < n; ++x) text_char(x, y, ' ', 0);
    text_at(0, y, s);
}

void text_clear(void)
{
    unsigned char y;
    for (y = 0; y < 24; ++y) text_line(y, "");
}

void text_mode(unsigned char wide)
{
    SW(0xC051);                    /* TEXT */
    SW(0xC052);                    /* full screen */
    SW(0xC054);
    SW(0xC00E);                    /* standard character set */
    SW(0xC05F);                    /* double hi-res off */
    wide_mode = wide;
    if (wide) {
        SW(0xC056);                /* lo-res: 80STORE cannot bank HGR */
        SW(0xC001);                /* 80STORE on */
        SW(0xC00D);                /* 80COL on */
    } else {
        SW(0xC00C);
        SW(0xC000);                /* 80STORE off (write, not key read) */
    }
    cat.visible = target.visible = 0;
    text_clear();
}

void video_init(void)
{
    unsigned char y;
    for (y = 0; y < 160; ++y)
        rows[y] = (unsigned char *)(0x2000U +
            ((unsigned int)(y & 7) << 10) +
            ((unsigned int)((y >> 3) & 7) << 7) + (y >> 6) * 40U);
    text_mode(0);
}

static void hide(Sprite *s)
{
    unsigned char y;
    if (!s->visible) return;
    for (y = 0; y < SPRITE_H; ++y)
        memcpy(rows[s->y + y] + s->x, s->saved[y], SPRITE_W);
    s->visible = 0;
}

static void show(Sprite *s, unsigned char x, unsigned char y)
{
    unsigned char i, j, *p;
    hide(s);
    s->x = x; s->y = y;
    for (i = 0; i < SPRITE_H; ++i) {
        p = rows[y + i] + x;
        memcpy(s->saved[i], p, SPRITE_W);
        for (j = 0; j < SPRITE_W; ++j)
            if (s->mask[i][j])
                p[j] = (p[j] & ~s->mask[i][j]) | s->bits[i][j];
    }
    s->visible = 1;
}

/* Pixel colors: 0=opaque black, 1=white, 2=orange artifact phase. */
static void pixel(Sprite *s, unsigned char x, unsigned char y,
                  unsigned char color)
{
    unsigned char b = x / 7, bit = 1 << (x % 7);
    s->mask[y][b] |= bit | 0x80;
    s->bits[y][b] &= ~bit;
    if (color == 1 || (color == 2 && (x & 1))) s->bits[y][b] |= bit;
    if (color == 2) s->bits[y][b] |= 0x80;
}

static unsigned char glyph(unsigned char c)
{
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '0' && c <= '9') return c - '0' + 26;
    return 36;
}

static void letters(Sprite *s, const char *label)
{
    unsigned char i, x, y, g;
    for (i = 0; label[i] && i < 8; ++i) {
        g = glyph(label[i]);
        for (y = 0; y < 7; ++y)
            for (x = 0; x < 5; ++x)
                if (font[g][y] & (16 >> x)) pixel(s, i * 6 + x, y, 1);
    }
}

void graphic_label(unsigned char column, unsigned char y, const char *s)
{
    unsigned char i, dx, dy, g, x;
    for (i = 0; s[i] && i < 36; ++i) {
        if (s[i] == ' ') continue;
        g = glyph(s[i]);
        for (dy = 0; dy < 7; ++dy) {
            for (dx = 0; dx < 5; ++dx) {
                x = i * 6 + dx;
                if (column + x / 7 < 40 && (font[g][dy] & (16 >> dx)))
                    rows[y + dy][column + x / 7] |= 1 << (x % 7);
            }
        }
    }
}

void graphics_mode(unsigned char mice)
{
    unsigned char y, x;
    text_mode(0);
    memset((void *)0x2000, 0, 8192);
    SW(0xC057);
    SW(0xC053);
    SW(0xC050);
    for (y = 0; y < 160; ++y) {
        rows[y][0] = rows[y][39] = 0x7F;
    }
    memset(rows[0], 0x7F, 40);
    memset(rows[159], 0x7F, 40);
    graphic_label(3, 12, mice ? "MOUSE CHASER" : "HOME ROW FISH MARKET");
    graphic_label(3, 30, mice ? "CATCH THE NEWSROOM PESTS" : "FRESH FISH FOR THE VILLAGE");
    for (x = 2; x < 38; ++x) rows[147][x] = 0x7F;
    if (mice) {
        for (y = 108; y < 147; ++y) {
            rows[y][37] = 0x7F;
            rows[y][38] = 0x7F;
        }
    } else {
        for (x = 10; x < 37; ++x) {
            rows[137][x] = 0x7F;
            rows[143][x] = (x & 1) ? 0x2A : 0x55;
        }
    }
    cat_show(POSE_IDLE);
}

void cat_show(unsigned char pose)
{
    unsigned char x, y, c;
    hide(&cat);
    memset(cat.bits, 0, sizeof(cat.bits));
    memset(cat.mask, 0, sizeof(cat.mask));
    for (y = 0; y < 24; ++y) {
        for (x = 0; x < 28; ++x) {
            c = cat_art[y][x];
            if (c != ' ') {
                if (pose == POSE_SLEEP && (y == 6 || y == 7) && c == 'W')
                    c = 'X';
                pixel(&cat, x, y, c == 'O' ? 2 : c == 'W' ? 1 : 0);
            }
        }
    }
    if (pose == POSE_POUNCE || pose == POSE_TYPE)
        for (y = 18; y < 22; ++y)
            for (x = 20; x < (pose == POSE_POUNCE ? 40 : 32); ++x)
                pixel(&cat, x, y, 2);
    if (pose == POSE_TYPO)
        for (y = 3; y < 12; ++y)
            for (x = 20; x < 24; ++x) pixel(&cat, x, y, 2);
    /* Even byte X keeps the orange pixel phase stable. */
    show(&cat, 2, pose == POSE_POUNCE ? 101 : 112);
}

void target_make(const char *label, unsigned char mouse)
{
    unsigned char x, y;
    hide(&target);
    memset(target.bits, 0, sizeof(target.bits));
    memset(target.mask, 0, sizeof(target.mask));
    letters(&target, label);
    for (y = 13; y < 23; ++y)
        for (x = 5; x < 26; ++x)
            if ((y > 14 && y < 21) || (x > 9 && x < 22))
                pixel(&target, x, y, 1);
    if (mouse) {
        for (y = 10; y < 16; ++y)
            for (x = 20; x < 25; ++x) pixel(&target, x, y, 1);
        for (x = 0; x < 7; ++x) pixel(&target, x, 21 - (x & 1), 1);
        pixel(&target, 23, 17, 0);
    } else {
        for (y = 14; y < 23; ++y)
            for (x = 0; x < 5; ++x)
                if (x < (y < 18 ? 18 - y : y - 18)) pixel(&target, x, y, 1);
        pixel(&target, 23, 17, 0);
        pixel(&target, 14, 17, 0);
        pixel(&target, 14, 18, 0);
    }
}

void target_show(unsigned char x, unsigned char y)
{
    show(&target, x, y);
}

void target_hide(void)
{
    hide(&target);
}