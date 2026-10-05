#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware.h"
#include "video.h"
#include "lessons.h"

static Stats stats;
static char line[81];
static unsigned char badges;
static unsigned int seed = 19;
static unsigned char aborted;
static unsigned char difficulty;

static void reset_stats(void)
{
    memset(&stats, 0, sizeof(stats));
    aborted = 0;
    key_flush();
}

static void hud(unsigned char mode, unsigned char round, unsigned int speed)
{
    sprintf(line, "%s %s %u/12 C:%u M:%u",
        mode ? "MICE" : "FISH", difficulty_names[difficulty],
        round + 1, stats.caught, stats.missed);
    text_line(20, line);
    sprintf(line, "Accuracy:%u%%  WPM:~%u  Speed:%u%%",
        accuracy(&stats), words_per_minute(&stats, refresh_hz), speed);
    text_line(21, line);
    text_line(23, "ESC menu  TAB pause  CTRL-S sound");
}

/* Common controls do not count as typing errors or typing time. */
static unsigned char controls(unsigned char k)
{
    if (k == KEY_ESC) { aborted = 1; return 1; }
    if (k == KEY_SOUND) { sound_on = !sound_on; return 1; }
    if (k == KEY_TAB) {
        text_line(23, "PAUSED - TAB resume / ESC menu");
        key_flush();
        do {
            wait_frame();
            k = key_read();
            if (k == KEY_ESC) { aborted = 1; break; }
            if (k == KEY_SOUND) sound_on = !sound_on;
        } while (k != KEY_TAB);
        text_line(23, "ESC menu  TAB pause  CTRL-S sound");
        return 1;
    }
    return 0;
}

static void results(unsigned char mode)
{
    unsigned char passed;
    passed = !aborted && accuracy(&stats) >= 80 &&
        stats.caught >= (mode == 2 ? STORY_COUNT : 8);
    if (passed) badges |= 1 << mode;
    text_mode(0);
    text_at(5, 2, "TYPER FELIX - LESSON REPORT");
    sprintf(line, "Difficulty: %s", difficulty_names[difficulty]);
    text_at(4, 3, line);
    text_at(2, 5, aborted ? "Lesson stopped. Keep practicing!" :
        passed ? "PURR-FECT! You earned a badge." : "Good practice! Try this lesson again.");
    sprintf(line, "Correct keys: %u", stats.correct); text_at(4, 8, line);
    sprintf(line, "Typing errors: %u", stats.errors); text_at(4, 10, line);
    sprintf(line, "Accuracy: %u%%", accuracy(&stats)); text_at(4, 12, line);
    sprintf(line, "Approximate WPM: %u", words_per_minute(&stats, refresh_hz));
    text_at(4, 14, line);
    sprintf(line, "%s: %u", mode == 2 ? "Lines printed" : "Targets caught", stats.caught);
    text_at(4, 16, line);
    text_at(4, 19, "RETURN to the village menu");
    text_at(2, 22, "Badges last until you quit the game.");
    key_flush();
    while (key_read() != 13) wait_frame();
}

static void choose_difficulty(unsigned char mode)
{
    unsigned char k;
    text_mode(0);
    text_at(4, 2, "CHOOSE YOUR DIFFICULTY");
    text_at(3, 4, mode == 0 ? "Home Row Fish Market" :
        mode == 1 ? "Mouse Chaser" : "Felix's Morning Gazette");
    text_at(3, 7, "1  Easy");
    text_at(3, 8, mode == 2 ? "Short lowercase phrases" :
        mode == 1 ? "Slower mice, 3-letter words" : "Slower fish, more time to type");
    text_at(3, 11, "2  Medium");
    text_at(3, 12, mode == 2 ? "Sentences with simple punctuation" :
        mode == 1 ? "Normal speed, 4-letter words" : "Normal fish speed");
    text_at(3, 15, "3  Challenge");
    text_at(3, 16, mode == 2 ? "Longer lines, numbers and symbols" :
        mode == 1 ? "Faster mice, 5-7 letter words" : "Faster fish, less time to type");
    text_at(3, 20, "Press 1, 2 or 3. ESC returns to menu.");
    key_wait_release();
    for (;;) {
        wait_frame(); ++seed;
        k = key_read();
        if (k == KEY_ESC) { aborted = 1; return; }
        if (k >= '1' && k <= '3') {
            difficulty = k - '1';
            return;
        }
    }
}

static void instruction(unsigned char mode)
{
    unsigned char k;
    choose_difficulty(mode);
    if (aborted) return;
    text_mode(0);
    text_at(4, 2, mode == 0 ? "1. HOME ROW FISH MARKET" :
        mode == 1 ? "2. MOUSE CHASER" : "3. FELIX'S MORNING GAZETTE");
    sprintf(line, "Difficulty: %s", difficulty_names[difficulty]);
    text_at(2, 3, line);
    if (mode == 0) {
        text_at(2, 5, "Rest your fingers on A S D F J K L ;");
        text_at(2, 7, "Type the letter carried by each fish.");
        text_at(2, 9, "A wrong key lets that fish splash away.");
        text_at(2, 11, "Catch it before it reaches Felix!");
    } else if (mode == 1) {
        text_at(2, 5, "Type each mouse's word, left to right.");
        text_at(2, 7, "Wrong key? Retry the same letter.");
        text_at(2, 9, "Catch mice before they reach the hole.");
        text_at(2, 11, "Under 5% errors per 4 words speeds");
        text_at(2, 12, "the next group up by 15% (max 240%).");
    } else {
        text_at(2, 5, "Requires an 80-column card.");
        text_at(2, 7, "Turn CAPS LOCK off. Use Shift!");
        text_at(2, 9, "Match case, spaces and punctuation.");
        text_at(2, 11, "Wrong keys do not print. Retry them.");
        text_at(2, 13, "RETURN sends a completed line to print.");
    }
    text_at(2, 16, mode == 2 ? "Badge: print all 8 lines, 80% accuracy." :
        "Badge: catch 8 of 12, 80% accuracy.");
    text_at(2, 19, "RETURN starts. ESC returns to menu.");
    text_at(2, 21, "TAB pauses. CTRL-S toggles sound.");
    key_wait_release();
    do { wait_frame(); ++seed; k = key_read(); } while (k != 13 && k != KEY_ESC);
    if (k == KEY_ESC) aborted = 1;
}

static void chase(unsigned char mice)
{
    unsigned char round, pos, len, k, x, done, pose_time;
    unsigned int movement, speed, group_good, group_bad;
    unsigned int before_good, before_bad, idle;
    const char *word;
    char fish[2];
    reset_stats();
    instruction(mice);
    if (aborted) return;
    srand(seed);
    graphics_mode(mice);
    speed = starting_speed(difficulty);
    group_good = group_bad = 0;
    for (round = 0; round < TARGET_COUNT && !aborted; ++round) {
        fish[0] = home_keys[(round + seed) % 8]; fish[1] = 0;
        if (mice) word = lesson_word(difficulty, rand());
        else word = fish;
        len = strlen(word); pos = 0;
        x = mice ? 10 : 30;
        movement = 0; done = pose_time = 0; idle = 0;
        before_good = stats.correct; before_bad = stats.errors;
        target_make(word, mice);
        target_show(x, 108);
        hud(mice, round, speed);
        sprintf(line, "Type: %s", word); text_line(22, line);
        while (!done && !aborted) {
            wait_frame(); ++stats.frames; ++idle;
            k = key_read();
            if (k && controls(k)) continue;
            if (k >= 32 && k != 127) {
                if (fold_case(k) == (unsigned char)word[pos]) {
                    ++stats.correct; ++pos;
                    sound_key();
                    if (pos == len) {
                        ++stats.caught; done = 1;
                        target_hide(); cat_show(POSE_POUNCE);
                        text_line(22, mice ? "Caught! Great typing." : "Fresh fish! Nice catch.");
                        sound_win();
                    } else {
                        sprintf(line, "Type: %s  Next: %c", word, word[pos]);
                        text_line(22, line);
                    }
                } else {
                    ++stats.errors; sound_error();
                    cat_show(POSE_TYPO); pose_time = 12;
                    if (!mice) {
                        ++stats.missed; done = 1;
                        target_hide();
                        text_line(22, "Splash! Reset fingers on the home row.");
                        sound_miss();
                    } else {
                        sprintf(line, "Try again. Next letter: %c", word[pos]);
                        text_line(22, line);
                    }
                }
                idle = 0;
            }
            if (!done) {
                movement += speed;
                if (movement >= (mice ? 1600U : 2200U)) {
                    movement -= mice ? 1600U : 2200U;
                    if (mice) ++x; else --x;
                    if ((mice && x > 31) || (!mice && x < 10)) {
                        target_hide(); ++stats.missed; done = 1;
                        cat_show(POSE_TYPO); sound_miss();
                        text_line(22, mice ? "Into the hole! Try the next word." :
                            "Splash! The fish slipped away.");
                    } else target_show(x, 108);
                }
                if (pose_time && --pose_time == 0) cat_show(POSE_IDLE);
                if (idle == 120) cat_show(POSE_SLEEP);
            }
        }
        group_good += stats.correct - before_good;
        group_bad += stats.errors - before_bad;
        if (mice && (round + 1) % 4 == 0) {
            if (group_good && (unsigned long)group_bad * 100UL <
                (unsigned long)(group_good + group_bad) * 5UL)
                speed = faster(speed);
            group_good = group_bad = 0;
        }
        if (!aborted) {
            hud(mice, round, speed);
            delay_frames(18); key_flush();
            cat_show(POSE_IDLE);
        }
    }
    target_hide();
    results(mice);
}

static void gazette_cat(unsigned char active)
{
    text_at(66, 1, " /\\_/\\      ");
    text_at(66, 2, active ? "( o.o ) TAP!" : "( -.- )     ");
    text_at(66, 3, " /|_|\\      ");
    text_at(66, 4, active ? " [==#==]    " : " [=====]    ");
}

static void gazette(void)
{
    unsigned char n, pos, len, k, j;
    const char *sentence;
    reset_stats();
    instruction(2);
    if (aborted) return;
    text_mode(1);
    text_at(2, 1, "FELIX'S MORNING GAZETTE");
    text_at(2, 3, "Village news, fresh fish, and very fast paws.");
    sprintf(line, "Difficulty: %s", difficulty_names[difficulty]);
    text_at(2, 4, line);
    gazette_cat(0);
    text_at(2, 6, "TODAY'S PRINTED EDITION");
    text_line(23, "ESC menu  TAB pause  CTRL-S sound");
    for (n = 0; n < STORY_COUNT && !aborted; ++n) {
        sentence = lesson_sentence(difficulty, n);
        pos = 0; len = strlen(sentence);
        sprintf(line, "Copy line %u of %u (CAPS LOCK off):", n + 1, STORY_COUNT);
        text_line(17, line);
        text_line(18, sentence);
        text_line(20, "");
        text_char(0, 20, ' ', 1);
        text_line(22, "Match the line exactly. RETURN prints when finished.");
        while (!aborted) {
            wait_frame(); ++stats.frames;
            k = key_read();
            if (!k) continue;
            if (controls(k)) continue;
            if (k == 13 && pos == len) {
                ++stats.caught;
                /* Rolling press: retain the latest six accepted lines. */
                for (j = 0; j < 6; ++j) {
                    text_line(8 + j, "");
                    if (n < 6) {
                        if (j <= n) text_at(2, 8 + j, lesson_sentence(difficulty, j));
                    } else text_at(2, 8 + j, lesson_sentence(difficulty, n - 5 + j));
                }
                sound_win(); gazette_cat(0); break;
            }
            if (pos < len && k == (unsigned char)sentence[pos]) {
                text_char(pos, 20, k, 0); ++pos; ++stats.correct;
                text_char(pos, 20, ' ', 1);
                gazette_cat(pos & 1);
                sound_key();
                if (pos == len) text_line(22, "Ready for the press! Hit RETURN.");
            } else if (k >= 32 || k == 13) {
                ++stats.errors; sound_error();
                text_line(22, pos == len ? "Line complete. Press RETURN to print." :
                    "Check case and punctuation, then retry the same character.");
            }
            sprintf(line, "Accuracy: %u%%     Approx. WPM: %u",
                accuracy(&stats), words_per_minute(&stats, refresh_hz));
            text_line(15, line);
        }
    }
    results(2);
}

static void menu(void)
{
    text_mode(0);
    text_at(5, 2, "TYPING WITH TYPER FELIX");
    text_at(12, 4, " /\\_/\\");
    text_at(12, 5, "( o.o )");
    text_at(12, 6, " > ^ <");
    text_at(3, 8, "Your shift at the village starts here.");
    text_at(3, 10, "1  Home Row Fish Market");
    text_at(3, 12, "2  Mouse Chaser");
    text_at(3, 14, "3  Morning Gazette (80 columns)");
    sprintf(line, "Badges: FISH[%c] MICE[%c] PRESS[%c]",
        badges & 1 ? '*' : ' ', badges & 2 ? '*' : ' ', badges & 4 ? '*' : ' ');
    text_at(3, 17, line);
    sprintf(line, "S Sound:%s  V Video:%uHz  Q Quit",
        sound_on ? "ON " : "OFF", refresh_hz);
    text_at(3, 20, line);
    text_at(3, 22, "Start with 1, then try 2 and 3.");
}

int main(void)
{
    unsigned char k;
    video_init();
    menu();
    for (;;) {
        wait_frame(); ++seed;
        k = fold_case(key_read());
        if (k == 'q') break;
        if (k == 's') { sound_on = !sound_on; menu(); }
        if (k == 'v') { refresh_hz = refresh_hz == 60 ? 50 : 60; menu(); }
        if (k == '1' || k == '2') { chase(k - '1'); menu(); }
        if (k == '3') { gazette(); menu(); }
    }
    text_mode(0);
    text_at(2, 10, "Thanks for helping Felix. See you soon!");
    return 0;
}