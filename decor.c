#include <ncurses.h>
#include <string.h>

void vimmy_decor(void) {
    initscr();
    noecho();
    curs_set(0);

    if (has_colors()) {
        start_color();
        use_default_colors();

        init_pair(1, COLOR_CYAN, -1);
        init_pair(2, COLOR_MAGENTA, -1);
        init_pair(3, COLOR_GREEN, -1);
        init_pair(4, COLOR_YELLOW, -1);
    }

    erase();

    int mid = COLS / 2;

    attron(COLOR_PAIR(1) | A_BOLD);

    mvprintw(2, mid - 17, "██╗   ██╗██╗███╗   ███╗███╗   ███╗██╗   ██╗");
    mvprintw(3, mid - 17, "██║   ██║██║████╗ ████║████╗ ████║╚██╗ ██╔╝");
    mvprintw(4, mid - 17, "╚██╗ ██╔╝██║██╔████╔██║██╔████╔██║ ╚████╔╝");
    mvprintw(5, mid - 17, " ╚████╔╝ ██║██║╚██╔╝██║██║╚██╔╝██║  ╚██╔╝");
    mvprintw(6, mid - 17, "  ╚═══╝  ╚═╝╚═╝ ╚═╝ ╚═╝╚═╝ ╚═╝ ╚═╝   ╚═╝");

    attroff(COLOR_PAIR(1) | A_BOLD);

    attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(9, mid - 12, "N E G I   T E X T   E D I T O R");
    attroff(COLOR_PAIR(2) | A_BOLD);

    attron(COLOR_PAIR(3));
    mvprintw(12, mid - 15, "[i]       Enter insert mode");
    mvprintw(13, mid - 15, "[ESC]     Return to normal mode");
    mvprintw(14, mid - 15, "[:w]      Save your file");
    mvprintw(15, mid - 15, "[:q]      Quit Vimmy");
    attroff(COLOR_PAIR(3));

    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(19, mid - 13, "PRESS ANY KEY TO START");
    attroff(COLOR_PAIR(4));

    mvprintw(22, mid - 12, "NEGI TECHNOLOGIES");
    refresh();

    getch();
    endwin();
}
