#ifndef FELIX_LESSONS_H
#define FELIX_LESSONS_H

#define TARGET_COUNT 12
#define STORY_COUNT 8
extern const char home_keys[];
extern const char * const mouse_words[];
extern const unsigned char mouse_word_count;
extern const char * const stories[];
extern const char * const difficulty_names[];
unsigned int starting_speed(unsigned char difficulty);
const char *lesson_word(unsigned char difficulty, unsigned int choice);
const char *lesson_sentence(unsigned char difficulty, unsigned char index);

typedef struct {
    unsigned int correct;
    unsigned int errors;
    unsigned int caught;
    unsigned int missed;
    unsigned long frames;
} Stats;

unsigned int accuracy(const Stats *s);
unsigned int words_per_minute(const Stats *s, unsigned char hz);
unsigned int faster(unsigned int speed);
unsigned char fold_case(unsigned char c);

#endif