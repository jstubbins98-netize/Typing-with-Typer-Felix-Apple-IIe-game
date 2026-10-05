#include "lessons.h"
#include <string.h>

const char * const difficulty_names[] = { "Easy", "Medium", "Challenge" };

static const char * const easy_stories[] = {
    "the cat naps", "a fish swims", "felix has a shop", "the sun is warm",
    "we like fresh fish", "a mouse runs home", "the cat has soft paws",
    "we can type the news"
};

static const char * const medium_stories[] = {
    "Felix opens his shop.", "The fish market is busy today.",
    "A small mouse runs past the desk.", "Please bring the morning paper.",
    "Felix has 3 fresh fish.", "Can you help the sleepy cat?",
    "We type the news, then print it.", "Good work! The paper is ready."
};

unsigned int starting_speed(unsigned char difficulty)
{
    if (difficulty == 0) return 65;
    if (difficulty == 2) return 140;
    return 100;
}

const char home_keys[] = "asdfjkl;";
const char * const mouse_words[] = {
    "cat", "run", "fish", "desk", "paw", "ask", "sad", "lad",
    "jazz", "milk", "news", "type", "jump", "quick", "market", "felix",
    "sleep", "mouse", "print", "village", "fetch", "kitten", "orange", "home"
};
const unsigned char mouse_word_count = sizeof(mouse_words) / sizeof(mouse_words[0]);

/* Each fits one 70-column print line, leaving room for Felix's masthead. */
const char * const stories[] = {
    "Felix opens the fish market at 7:30 AM.",
    "\"Fresh fish!\" he calls. Today, 12 trout cost $24.",
    "A quick brown mouse jumps past the sleepy cat.",
    "Stop! Who left 3 paw prints on my desk?",
    "Felix's news: rain on Monday; sunshine on Tuesday.",
    "We need eggs, milk, bread, and 2 bags of rice.",
    "Type (carefully), use Shift, and check each comma.",
    "The Gazette is ready! Great work, apprentice."
};

const char *lesson_word(unsigned char difficulty, unsigned int choice)
{
    unsigned char i, count = 0, length;
    for (i = 0; i < mouse_word_count; ++i) {
        length = strlen(mouse_words[i]);
        if ((difficulty == 0 && length <= 3) ||
            (difficulty == 1 && length == 4) ||
            (difficulty == 2 && length >= 5)) ++count;
    }
    choice %= count;
    for (i = 0; i < mouse_word_count; ++i) {
        length = strlen(mouse_words[i]);
        if ((difficulty == 0 && length <= 3) ||
            (difficulty == 1 && length == 4) ||
            (difficulty == 2 && length >= 5)) {
            if (!choice) return mouse_words[i];
            --choice;
        }
    }
    return mouse_words[0];
}

const char *lesson_sentence(unsigned char difficulty, unsigned char index)
{
    if (difficulty == 0) return easy_stories[index];
    if (difficulty == 1) return medium_stories[index];
    return stories[index];
}

unsigned int accuracy(const Stats *s)
{
    unsigned int total = s->correct + s->errors;
    if (!total) return 100;
    return (unsigned int)((unsigned long)s->correct * 100UL / total);
}

unsigned int words_per_minute(const Stats *s, unsigned char hz)
{
    if (!s->frames) return 0;
    return (unsigned int)((unsigned long)s->correct * hz * 12UL / s->frames);
}

unsigned int faster(unsigned int speed)
{
    unsigned int next = (speed * 115U + 50U) / 100U;
    return next > 240U ? 240U : next;
}

unsigned char fold_case(unsigned char c)
{
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}