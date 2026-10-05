#include <assert.h>
#include <string.h>
#include "../src/lessons.h"

int main(void)
{
    unsigned char level, n;
    unsigned int choice, length;
    assert(starting_speed(0) == 65);
    assert(starting_speed(1) == 100);
    assert(starting_speed(2) == 140);
    for (level = 0; level < 3; ++level) {
        for (choice = 0; choice < 256; ++choice) {
            length = strlen(lesson_word(level, choice));
            if (level == 0) assert(length == 3);
            if (level == 1) assert(length == 4);
            if (level == 2) assert(length >= 5 && length <= 7);
        }
        for (n = 0; n < STORY_COUNT; ++n) {
            length = strlen(lesson_sentence(level, n));
            assert(length > 0 && length <= 70);
            if (level == 0) assert(length < strlen(lesson_sentence(2, n)));
        }
    }
    return 0;
}