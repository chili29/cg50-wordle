#include <gint/gint.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/keycodes.h>
#include <gint/rtc.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "words.h"

#define ROWS 6
#define COLS 5
#define STATS_FILE "/WORDLE.DAT"
#define STATS_MAGIC 0x574F5244u
#define STATS_VERSION 1u
#define WORD_COUNT (sizeof(words) / sizeof(words[0]))

/* words[] is generated from the English five-letter dictionary by make_word_header.py. */

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t games;
    uint32_t wins;
    uint32_t losses;
    uint32_t current_streak;
    uint32_t best_streak;
    uint32_t guess_distribution[ROWS];
} stats_t;

static stats_t stats;
static char answer[6];
static char guesses[ROWS][6];
static unsigned char marks[ROWS][COLS];
static int row, cursor, ended, won, stats_screen;
static char current[6];
static char message[48];

/* Map the calculator's physical letter-labelled keys directly to letters.
   This deliberately ignores the ALPHA modifier, so each letter is one press. */
static char key_to_letter(int key)
{
    switch(key) {
    case KEY_XOT:     return 'a';
    case KEY_LOG:     return 'b';
    case KEY_LN:      return 'c';
    case KEY_SIN:     return 'd';
    case KEY_COS:     return 'e';
    case KEY_TAN:     return 'f';
    case KEY_LEFTP:   return 'g';
    case KEY_RIGHTP:  return 'h';
    case KEY_COMMA:   return 'i';
    case KEY_ARROW:   return 'j';
    case KEY_7:       return 'k';
    case KEY_8:       return 'l';
    case KEY_9:       return 'm';
    case KEY_DEL:     return 'n';
    case KEY_4:       return 'o';
    case KEY_5:       return 'p';
    case KEY_6:       return 'q';
    case KEY_MUL:     return 'r';
    case KEY_DIV:     return 's';
    case KEY_1:       return 't';
    case KEY_2:       return 'u';
    case KEY_3:       return 'v';
    case KEY_ADD:     return 'w';
    case KEY_SUB:     return 'x';
    case KEY_0:       return 'y';
    case KEY_DOT:     return 'z';
    default:          return 0;
    }
}

static int tile_color(int mark)
{
    if(mark == 2) return C_RGB(7, 19, 9);   /* green */
    if(mark == 1) return C_RGB(23, 19, 5);  /* yellow */
    return C_RGB(9, 10, 12);                /* gray */
}

static bool valid_word(const char *s)
{
    int low = 0;
    int high = (int)WORD_COUNT - 1;

    while(low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = strcmp(s, words[mid]);
        if(cmp == 0) return true;
        if(cmp < 0) high = mid - 1;
        else low = mid + 1;
    }
    return false;
}

static void load_stats(void)
{
    FILE *fp = fopen(STATS_FILE, "rb");
    if(fp) {
        stats_t loaded;
        size_t n = fread(&loaded, 1, sizeof loaded, fp);
        fclose(fp);
        if(n == sizeof loaded && loaded.magic == STATS_MAGIC &&
           loaded.version == STATS_VERSION) {
            stats = loaded;
            return;
        }
    }
    memset(&stats, 0, sizeof stats);
    stats.magic = STATS_MAGIC;
    stats.version = STATS_VERSION;
}

static void save_stats(void)
{
    FILE *fp = fopen(STATS_FILE, "wb");
    if(!fp) {
        strcpy(message, "STATS SAVE FAILED");
        return;
    }
    fwrite(&stats, 1, sizeof stats, fp);
    fflush(fp);
    fclose(fp);
}

static void finish_game(bool did_win)
{
    if(ended) return;
    ended = 1;
    won = did_win;
    stats.games++;
    if(did_win) {
        stats.wins++;
        stats.current_streak++;
        if(stats.current_streak > stats.best_streak)
            stats.best_streak = stats.current_streak;
        if(row >= 0 && row < ROWS) stats.guess_distribution[row]++;
        strcpy(message, "YOU GOT IT!");
    } else {
        stats.losses++;
        stats.current_streak = 0;
        strcpy(message, answer);
    }
    save_stats();
}

static void new_game(void)
{
    unsigned int seed = (unsigned int)rtc_ticks();
    seed ^= (unsigned int)(uintptr_t)&seed;
    seed ^= (unsigned int)(stats.games * 2654435761u);
    srand(seed);
    strcpy(answer, words[rand() % WORD_COUNT]);
    memset(guesses, 0, sizeof guesses);
    memset(marks, 0, sizeof marks);
    strcpy(current, "aaaaa");
    row = 0;
    cursor = 0;
    ended = 0;
    won = 0;
    stats_screen = 0;
    message[0] = 0;
}

static void score_guess(int r)
{
    int remaining[26] = {0};
    for(int i = 0; i < COLS; i++) marks[r][i] = 0;

    /* Exact matches first, then consume remaining answer letters. */
    for(int i = 0; i < COLS; i++) {
        if(guesses[r][i] == answer[i]) marks[r][i] = 2;
        else remaining[answer[i] - 'a']++;
    }
    for(int i = 0; i < COLS; i++) {
        if(marks[r][i] == 2) continue;
        int k = guesses[r][i] - 'a';
        if(k >= 0 && k < 26 && remaining[k] > 0) {
            marks[r][i] = 1;
            remaining[k]--;
        }
    }
}

static void draw_tile(int x, int y, char letter, int mark, bool selected)
{
    int fill = mark ? tile_color(mark) : C_RGB(5, 6, 8);
    drect(x, y, x + 35, y + 21, fill);
    drect_border(x, y, x + 35, y + 21, fill, selected ? 2 : 1,
                 selected ? C_RGB(31, 31, 31) : C_RGB(13, 14, 16));
    char s[2] = { letter ? (char)(letter - 'a' + 'A') : ' ', 0 };
    dtext_opt(x + 18, y + 4, C_WHITE, C_NONE,
              DTEXT_CENTER, DTEXT_TOP, s, -1);
}

static void draw_game(void)
{
    dclear(C_RGB(2, 3, 4));
    drect(0, 0, 395, 25, C_RGB(6, 7, 9));
    dtext(12, 7, C_WHITE, "WORDLE");
    dtext(320, 7, C_RGB(21, 22, 23), "FX-CG50");

    for(int r = 0; r < ROWS; r++) {
        for(int c = 0; c < COLS; c++) {
            char letter = 0;
            int mark = 0;
            bool selected = false;
            if(r < row) {
                letter = guesses[r][c];
                mark = marks[r][c];
            } else if(r == row && !ended) {
                letter = current[c];
                selected = (c == cursor);
            } else if(r == row && ended && guesses[r][c]) {
                letter = guesses[r][c];
                mark = marks[r][c];
            }
            draw_tile(90 + c * 42, 37 + r * 26, letter, mark, selected);
        }
    }

    if(message[0]) {
        dtext_opt(198, 207, won ? C_RGB(10, 28, 14) : C_RGB(29, 22, 9),
                  C_NONE, DTEXT_CENTER, DTEXT_TOP, message, -1);
    } else {
        dtext_opt(198, 207, C_RGB(20, 21, 23), C_NONE,
                  DTEXT_CENTER, DTEXT_TOP, "F1 STATS   EXE GUESS   EXIT QUIT", -1);
    }
    if(ended) {
        dtext_opt(198, 192, C_WHITE, C_NONE, DTEXT_CENTER, DTEXT_TOP,
                  "EXE: NEW GAME", -1);
    } else if(message[0]) {
        dtext_opt(198, 192, C_WHITE, C_NONE, DTEXT_CENTER, DTEXT_TOP,
                  "WORD NOT IN LIST", -1);
    } else {
        dtext_opt(198, 192, C_RGB(17, 18, 20), C_NONE,
                  DTEXT_CENTER, DTEXT_TOP, "LETTER KEYS TYPE  ARROWS MOVE", -1);
    }
    dupdate();
}

static void draw_stats(void)
{
    char line[64];
    unsigned int pct = stats.games ? (stats.wins * 100u) / stats.games : 0;
    dclear(C_RGB(2, 3, 4));
    drect(0, 0, 395, 27, C_RGB(6, 7, 9));
    dtext(12, 8, C_WHITE, "WORDLE STATISTICS");
    snprintf(line, sizeof line, "Games played: %lu", (unsigned long)stats.games);
    dtext(35, 43, C_WHITE, line);
    snprintf(line, sizeof line, "Wins: %lu", (unsigned long)stats.wins);
    dtext(35, 65, C_RGB(12, 28, 15), line);
    snprintf(line, sizeof line, "Losses: %lu", (unsigned long)stats.losses);
    dtext(35, 87, C_WHITE, line);
    snprintf(line, sizeof line, "Win percentage: %u%%", pct);
    dtext(35, 109, C_WHITE, line);
    snprintf(line, sizeof line, "Current streak: %lu", (unsigned long)stats.current_streak);
    dtext(35, 131, C_WHITE, line);
    snprintf(line, sizeof line, "Best streak: %lu", (unsigned long)stats.best_streak);
    dtext(35, 153, C_WHITE, line);
    dtext(35, 177, C_WHITE, "WINS BY NUMBER OF GUESSES:");
    snprintf(line, sizeof line, "1:%lu  2:%lu  3:%lu  4:%lu",
        (unsigned long)stats.guess_distribution[0],
        (unsigned long)stats.guess_distribution[1],
        (unsigned long)stats.guess_distribution[2],
        (unsigned long)stats.guess_distribution[3]);
    dtext(35, 190, C_WHITE, line);
    snprintf(line, sizeof line, "5:%lu  6:%lu",
        (unsigned long)stats.guess_distribution[4],
        (unsigned long)stats.guess_distribution[5]);
    dtext(35, 202, C_WHITE, line);
    dupdate();
}

static void show_stats(void)
{
    stats_screen = 1;
    while(stats_screen) {
        draw_stats();
        key_event_t ev = getkey();
        if(ev.type != KEYEV_DOWN) continue;
        if(ev.key == KEY_EXIT || ev.key == KEY_F1) stats_screen = 0;
    }
}

int main(void)
{
    gint_setrestart(1);
    load_stats();
    new_game();

    while(1) {
        draw_game();
        key_event_t ev = getkey();
        if(ev.type != KEYEV_DOWN && ev.type != KEYEV_HOLD) continue;

        if(ev.key == KEY_F1) {
            show_stats();
            continue;
        }
        if(ended) {
            if(ev.key == KEY_EXE) new_game();
            else if(ev.key == KEY_EXIT) return 0;
            continue;
        }

        /* SHIFT+DEL clears the selected tile; DEL alone types N. */
        if(ev.shift && ev.key == KEY_DEL) {
            current[cursor] = 'a';
            message[0] = 0;
            continue;
        }

        /* Letter-labelled physical keys type directly, without ALPHA. */
        char letter = key_to_letter(ev.key);
        if(letter) {
            current[cursor] = letter;
            cursor = (cursor + 1) % COLS;
            message[0] = 0;
            continue;
        }

        switch(ev.key) {
        case KEY_UP:
            current[cursor] = (current[cursor] == 'z') ? 'a' : current[cursor] + 1;
            message[0] = 0;
            break;
        case KEY_DOWN:
            current[cursor] = (current[cursor] == 'a') ? 'z' : current[cursor] - 1;
            message[0] = 0;
            break;
        case KEY_LEFT:
            cursor = (cursor + COLS - 1) % COLS;
            break;
        case KEY_RIGHT:
            cursor = (cursor + 1) % COLS;
            break;
        case KEY_EXE:
            if(!valid_word(current)) {
                strcpy(message, "WORD NOT IN LIST");
                break;
            }
            strcpy(guesses[row], current);
            score_guess(row);
            if(!strcmp(current, answer)) {
                finish_game(true);
            } else if(row == ROWS - 1) {
                finish_game(false);
            } else {
                row++;
                strcpy(current, "aaaaa");
                cursor = 0;
                message[0] = 0;
            }
            break;
        case KEY_EXIT:
            return 0;
        default:
            break;
        }
    }
    return 0;
}
