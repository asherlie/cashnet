#include <unistd.h>
#include <stdlib.h>
#include <ncurses.h>

struct window{
    WINDOW* w;
    int lines;
};

void append_window(struct window* w, char* str){
    mvwprintw(w->w, w->lines++, 1, str);
    wrefresh(w->w);
}

void init_windows(struct window* messages, struct window* text, int text_lines){
    int x, y;
    messages->lines = text->lines = 1;
    messages->w = malloc(sizeof(WINDOW));
    text->w = malloc(sizeof(WINDOW));
    messages->w = initscr();
    getmaxyx(messages->w, y, x);
    text->w = newwin(text_lines, x, y-text_lines, 0);
    // how to resize relative to old dims
    wresize(messages->w, y-text_lines, x);
    refresh();
    box(text->w, 0, 0);
    box(messages->w, 0, 0);
    mvwprintw(messages->w, 0, 1, "MESSAGES");
    mvwprintw(text->w, 0, 1, "HELLO");
    wrefresh(messages->w);
    wrefresh(text->w);
}

int main(){
    struct window msg, txt;
    init_windows(&msg, &txt, 6);
    getchar();
    endwin();
    return 0;
}
