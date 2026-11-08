#define _XOPEN_SOURCE 700

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#define MAX_LINES 100000

typedef struct {
    char **lines;
    int count;
    int capacity;
} Editor;

Editor editor = {0};

int cx = 0, cy = 0;
int row_offset = 0;
int insert_mode = 0;
int modified = 0;

char filename[1024] = "[No Name]";
char message[256] = "VIMMY | i: Insert | :help: Help";

void reserve(int n) {
    if (editor.capacity >= n) return;

    int size = editor.capacity ? editor.capacity * 2 : 32;

    while (size < n) size *= 2;

    editor.lines = realloc(
        editor.lines,
        size * sizeof(char *)
    );

    if (!editor.lines) exit(1);

    editor.capacity = size;
}

void add_line(int y, const char *text) {
    reserve(editor.count + 1);

    for (int i = editor.count; i > y; i--)
        editor.lines[i] = editor.lines[i - 1];

    editor.lines[y] = strdup(text);

    editor.count++;
    modified = 1;
}

void remove_line(int y) {
    if (editor.count <= 1) return;

    free(editor.lines[y]);

    for (int i = y; i < editor.count - 1; i++)
        editor.lines[i] = editor.lines[i + 1];

    editor.count--;
    modified = 1;
}

int line_length(int y) {
    return strlen(editor.lines[y]);
}

void initialize(void) {
    reserve(1);

    editor.lines[0] = strdup("");
    editor.count = 1;
}

void load_file(const char *path) {
    FILE *file = fopen(path, "r");

    if (!file) return;

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
}

void save_file(void) {
    if (!strcmp(filename, "[No Name]")) {
        strcpy(message, "Use :w filename");
        return;
    }

    FILE *file = fopen(filename, "w");

    if (!file) {
        strcpy(message, "Cannot save file!");
        return;
    }

    for (int i = 0; i < editor.count; i++)
        fprintf(file, "%s\n", editor.lines[i]);

    fclose(file);

    modified = 0;

    snprintf(message, sizeof(message),
             "Saved %s", filename);
}

void draw(void) {
    int height, width;

    getmaxyx(stdscr, height, width);

    erase();

    for (int i = 0; i < height - 2; i++) {
        int y = row_offset + i;

        if (y >= editor.count) break;

        attron(COLOR_PAIR(2));

        mvprintw(i, 0, "%4d ", y + 1);

        attroff(COLOR_PAIR(2));

        mvaddnstr(
            i,
            5,
            editor.lines[y],
            width - 6
        );
    }

    attron(COLOR_PAIR(1) | A_BOLD);

    mvhline(height - 2, 0, ' ', width);

    mvprintw(
        height - 2,
        0,
        " %s %s | %s | %d:%d",
        filename,
        modified ? "[+]" : "",
        insert_mode ? "INSERT" : "NORMAL",
        cy + 1,
        cx + 1
    );

    attroff(COLOR_PAIR(1) | A_BOLD);

    attron(A_DIM);

    mvhline(height - 1, 0, ' ', width);
    mvaddnstr(height - 1, 0, message, width);

    attroff(A_DIM);

    move(cy - row_offset, cx + 5);

    refresh();
}

void insert_character(int ch) {
    int len = line_length(cy);

    char *line = malloc(len + 2);

    memcpy(line, editor.lines[cy], cx);

    line[cx] = ch;

    memcpy(
        line + cx + 1,
        editor.lines[cy] + cx,
        len - cx + 1
    );

    free(editor.lines[cy]);

    editor.lines[cy] = line;

    cx++;
    modified = 1;
}

void backspace(void) {
    if (cx > 0) {
        memmove(
            editor.lines[cy] + cx - 1,
            editor.lines[cy] + cx,
            strlen(editor.lines[cy] + cx) + 1
        );

        cx--;
        modified = 1;
    }
}

void new_line(void) {
    char *tail = strdup(editor.lines[cy] + cx);

    editor.lines[cy][cx] = '\0';

    add_line(cy + 1, tail);

    free(tail);

    cy++;
    cx = 0;
}

void command(void) {
    int height, width;

    getmaxyx(stdscr, height, width);

    char input[1024] = {0};

    echo();

    move(height - 1, 0);
    clrtoeol();
    addch(':');

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
        strncpy(filename, input + 2, sizeof(filename) - 1);
        save_file();
    }

    else if (!strcmp(input, "wq")) {
        save_file();

        if (!modified) {
            endwin();
            exit(0);
        }
    }

    else if (!strcmp(input, "help")) {
        strcpy(message,
        "i insert | Esc normal | h j k l move | x delete | dd delete line | :w save | :q quit");
    }

    else {
        strcpy(message, "Unknown command");
    }
}

void normal_mode(int ch) {
    switch (ch) {
        case 'i':
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
            if (cx > line_length(cy))
                cx = line_length(cy);
            break;

        case 'k':
        case KEY_UP:
            if (cy > 0) cy--;
            if (cx > line_length(cy))
                cx = line_length(cy);
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
            new_line();
            insert_mode = 1;
            break;

        case ':':
            command();
            break;

        case 'd':
            if (getch() == 'd') {
                remove_line(cy);

                if (cy >= editor.count)
                    cy = editor.count - 1;

                cx = 0;
            }
            break;

        case 'G':
            cy = editor.count - 1;
            cx = 0;
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
        strncpy(filename, argv[1], sizeof(filename) - 1);
        load_file(filename);
    }

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

        if (cy < row_offset)
            row_offset = cy;

        if (cy >= row_offset + LINES - 2)
            row_offset = cy - LINES + 3;

        draw();

        ch = getch();

        if (insert_mode) {
            if (ch == 27) {
                insert_mode = 0;
            }

            else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                backspace();
            }

            else if (ch == '\n' || ch == KEY_ENTER) {
                new_line();
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
