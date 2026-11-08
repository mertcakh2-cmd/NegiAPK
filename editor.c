#define _DEFAULT_SOURCE

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#define MAX_LINES 100000
#define MAX_LINE 65536

typedef struct {
    char **lines;
    int count;
    int capacity;
} Editor;

static Editor editor = {0};

static int cx = 0;
static int cy = 0;
static int row_offset = 0;
static int col_offset = 0;

static int insert_mode = 0;
static int modified = 0;

static char filename[1024] = "[No Name]";
static char message[256] = "NEGI VIMMY | Press i to insert | :help for help";

void vimmy_decor(void);

static void reserve(int n) {
    if (editor.capacity >= n) return;

    int size = editor.capacity ? editor.capacity * 2 : 32;

    while (size < n) size *= 2;

    char **p = realloc(editor.lines, size * sizeof(char *));

    if (!p) {
        endwin();
        exit(1);
    }

    editor.lines = p;
    editor.capacity = size;
}

static int line_length(int y) {
    return strlen(editor.lines[y]);
}

static void add_line(int y, const char *text) {
    if (editor.count >= MAX_LINES) return;

    reserve(editor.count + 1);

    for (int i = editor.count; i > y; i--)
        editor.lines[i] = editor.lines[i - 1];

    editor.lines[y] = strdup(text);

    if (!editor.lines[y]) exit(1);

    editor.count++;
    modified = 1;
}

static void delete_line(int y) {
    if (editor.count <= 1) {
        editor.lines[0][0] = '\0';
        modified = 1;
        return;
    }

    free(editor.lines[y]);

    for (int i = y; i < editor.count - 1; i++)
        editor.lines[i] = editor.lines[i + 1];

    editor.count--;
    modified = 1;
}

static void initialize(void) {
    reserve(1);

    editor.lines[0] = strdup("");
    editor.count = 1;
}

static void load_file(const char *path) {
    FILE *file = fopen(path, "r");

    if (!file) {
        snprintf(message, sizeof(message), "New file: %s", path);
        return;
    }

    free(editor.lines[0]);
    editor.count = 0;

    char *line = NULL;
    size_t capacity = 0;

    while (getline(&line, &capacity, file) != -1) {
        line[strcspn(line, "\r\n")] = '\0';

        add_line(editor.count, line);
    }

    free(line);
    fclose(file);

    if (editor.count == 0)
        add_line(0, "");

    modified = 0;

    snprintf(message, sizeof(message), "Opened %s", path);
}

static void save_file(void) {
    if (!strcmp(filename, "[No Name]")) {
        strcpy(message, "Use :w filename to save");
        return;
    }

    FILE *file = fopen(filename, "w");

    if (!file) {
        strcpy(message, "Could not save file");
        return;
    }

    for (int i = 0; i < editor.count; i++)
        fprintf(file, "%s\n", editor.lines[i]);

    fclose(file);

    modified = 0;

    snprintf(message, sizeof(message), "Saved %s", filename);
}

static void clamp_cursor(void) {
    if (cy < 0) cy = 0;
    if (cy >= editor.count) cy = editor.count - 1;

    if (cx < 0) cx = 0;
    if (cx > line_length(cy)) cx = line_length(cy);
}

static void insert_character(int ch) {
    int len = line_length(cy);

    if (len >= MAX_LINE - 2) return;

    char *line = malloc((size_t)len + 2);

    if (!line) exit(1);

    memcpy(line, editor.lines[cy], (size_t)cx);

    line[cx] = (char)ch;

    memcpy(
        line + cx + 1,
        editor.lines[cy] + cx,
        (size_t)(len - cx + 1)
    );

    free(editor.lines[cy]);

    editor.lines[cy] = line;

    cx++;
    modified = 1;
}

static void backspace(void) {
    if (cx > 0) {
        memmove(
            editor.lines[cy] + cx - 1,
            editor.lines[cy] + cx,
            strlen(editor.lines[cy] + cx) + 1
        );

        cx--;
        modified = 1;
    }
    else if (cy > 0) {
        int previous_length = line_length(cy - 1);
        int current_length = line_length(cy);

        char *joined = malloc(
            (size_t)previous_length + current_length + 1
        );

        if (!joined) exit(1);

        strcpy(joined, editor.lines[cy - 1]);
        strcat(joined, editor.lines[cy]);

        free(editor.lines[cy - 1]);
        editor.lines[cy - 1] = joined;

        delete_line(cy);

        cy--;
        cx = previous_length;
    }
}

static void new_line(void) {
    char *tail = strdup(editor.lines[cy] + cx);

    if (!tail) exit(1);

    editor.lines[cy][cx] = '\0';

    add_line(cy + 1, tail);

    free(tail);

    cy++;
    cx = 0;
}

static void draw(void) {
    int height, width;

    getmaxyx(stdscr, height, width);

    erase();

    int text_height = height - 2;

    for (int i = 0; i < text_height; i++) {
        int y = row_offset + i;

        if (y >= editor.count) break;

        attron(COLOR_PAIR(2));
        mvprintw(i, 0, "%4d ", y + 1);
        attroff(COLOR_PAIR(2));

        int len = line_length(y);

        if (col_offset < len) {
            mvaddnstr(
                i,
                5,
                editor.lines[y] + col_offset,
                width - 6
            );
        }
        else if (len == 0) {
            attron(A_DIM);
            mvaddch(i, 5, '~');
            attroff(A_DIM);
        }
    }

    attron(COLOR_PAIR(1) | A_BOLD);

    mvhline(height - 2, 0, ' ', width);

    mvprintw(
        height - 2,
        0,
        " %s%s | %s | NEGI VIMMY | %d:%d",
        filename,
        modified ? " [+]" : "",
        insert_mode ? "INSERT" : "NORMAL",
        cy + 1,
        cx + 1
    );

    attroff(COLOR_PAIR(1) | A_BOLD);

    attron(A_DIM);

    mvhline(height - 1, 0, ' ', width);
    mvaddnstr(height - 1, 0, message, width);

    attroff(A_DIM);

    int screen_y = cy - row_offset;
    int screen_x = 5 + cx - col_offset;

    if (screen_y >= 0 && screen_y < text_height &&
        screen_x >= 5 && screen_x < width) {
        move(screen_y, screen_x);
    }

    refresh();
}

static void ensure_visible(void) {
    int height, width;

    getmaxyx(stdscr, height, width);

    int text_height = height - 2;

    if (cy < row_offset)
        row_offset = cy;

    if (cy >= row_offset + text_height)
        row_offset = cy - text_height + 1;

    if (cx < col_offset)
        col_offset = cx;

    if (cx >= col_offset + width - 6)
        col_offset = cx - width + 7;

    if (col_offset < 0)
        col_offset = 0;
}

static void command(void) {
    int height, width;

    getmaxyx(stdscr, height, width);

    char input[1024] = {0};

    move(height - 1, 0);
    clrtoeol();
    addch(':');

    echo();
    curs_set(1);

    getnstr(input, sizeof(input) - 1);

    noecho();

    if (!strcmp(input, "q")) {
        if (modified) {
            strcpy(message, "Unsaved changes! Use :q!");
        } else {
            endwin();
            exit(0);
        }
    }

    else if (!strcmp(input, "q!")) {
        endwin();
        exit(0);
    }

    else if (!strcmp(input, "w")) {
        save_file();
    }

    else if (!strncmp(input, "w ", 2)) {
        snprintf(filename, sizeof(filename), "%s", input + 2);
        save_file();
    }

    else if (!strcmp(input, "wq") || !strcmp(input, "x")) {
        save_file();

        if (!modified) {
            endwin();
            exit(0);
        }
    }

    else if (!strcmp(input, "help")) {
        strcpy(
            message,
            "i insert | Esc normal | h j k l move | dd delete | :w save | :q quit"
        );
    }

    else {
        strcpy(message, "Unknown command");
    }
}

static void normal_mode(int ch) {
    switch (ch) {
        case 'i':
            insert_mode = 1;
            strcpy(message, "-- INSERT --");
            break;

        case 'a':
            if (cx < line_length(cy)) cx++;
            insert_mode = 1;
            break;

        case 'h':
        case KEY_LEFT:
            if (cx > 0) cx--;
            break;

        case 'l':
        case KEY_RIGHT:
            if (cx < line_length(cy)) cx++;
            break;

        case 'j':
        case KEY_DOWN:
            if (cy < editor.count - 1) cy++;
            clamp_cursor();
            break;

        case 'k':
        case KEY_UP:
            if (cy > 0) cy--;
            clamp_cursor();
            break;

        case '0':
            cx = 0;
            break;

        case '$':
            cx = line_length(cy);
            break;

        case 'x':
            if (cx < line_length(cy)) {
                memmove(
                    editor.lines[cy] + cx,
                    editor.lines[cy] + cx + 1,
                    strlen(editor.lines[cy] + cx)
                );

                modified = 1;
            }
            break;

        case 'o':
            cx = line_length(cy);
            new_line();
            insert_mode = 1;
            break;

        case 'O':
            cx = 0;
            new_line();

            {
                char *tmp = editor.lines[cy];
                editor.lines[cy] = editor.lines[cy - 1];
                editor.lines[cy - 1] = tmp;
            }

            cy--;
            insert_mode = 1;
            break;

        case ':':
            command();
            break;

        case 'd':
            if (getch() == 'd') {
                delete_line(cy);
                clamp_cursor();
                cx = 0;
            }
            break;

        case 'G':
            cy = editor.count - 1;
            clamp_cursor();
            break;

        case 'g':
            if (getch() == 'g') {
                cy = 0;
                cx = 0;
            }
            break;
    }
}

int main(int argc, char **argv) {
    setlocale(LC_ALL, "");

    initialize();

    if (argc > 1) {
        snprintf(filename, sizeof(filename), "%s", argv[1]);
        load_file(filename);
    }

    /* NEGI welcome screen */
    vimmy_decor();

    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);

    if (has_colors()) {
        start_color();
        use_default_colors();

        init_pair(1, COLOR_BLACK, COLOR_CYAN);
        init_pair(2, COLOR_CYAN, -1);
    }

    while (1) {
        int ch;

        clamp_cursor();
        ensure_visible();
        draw();

        ch = getch();

        if (ch == KEY_RESIZE)
            continue;

        if (insert_mode) {
            if (ch == 27) {
                insert_mode = 0;
                if (cx > 0) cx--;
                strcpy(message, "NORMAL");
            }

            else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                backspace();
            }

            else if (ch == '\n' || ch == KEY_ENTER) {
                new_line();
            }

            else if (ch == KEY_LEFT) {
                if (cx > 0) cx--;
            }

            else if (ch == KEY_RIGHT) {
                if (cx < line_length(cy)) cx++;
            }

            else if (ch == KEY_UP) {
                if (cy > 0) cy--;
                clamp_cursor();
            }

            else if (ch == KEY_DOWN) {
                if (cy < editor.count - 1) cy++;
                clamp_cursor();
            }

            else if (ch >= 32 && ch <= 255) {
                insert_character(ch);
            }
        }

        else {
            normal_mode(ch);
        }
    }

    endwin();

    return 0;
}
