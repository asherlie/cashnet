#include <unistd.h>
#include <ncurses.h>

int main(){
    int x, y, text_lines = 5;
    WINDOW* text, * messages = initscr();
    /*
     * printw("hello");
     * mvprintw(10, 30, "foooy");
     * refresh();
     * usleep(1000000);
     * endwin();
    */
    getmaxyx(messages, y, x);
    /*win = newwin(10, y, x-10, y-10);*/
    /*text = newwin(10, y, y-10, x-10);*/
    text = newwin(text_lines, x, y-text_lines, 0);
    // how to resize relative to old dims
    wresize(messages, y-text_lines, x);
    refresh();
    box(text, 0, 0);
    box(messages, 0, 0);
    mvwprintw(messages, 0, 1, "MESSAGES");
    mvwprintw(text, 0, 1, "HELLO");
    wrefresh(messages);
    wrefresh(text);
    /*mvprintw(10, 30, "foooy");*/
    getchar();
    endwin();
    return 0;
}
