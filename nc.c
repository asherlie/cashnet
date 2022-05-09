#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <sys/param.h>
#include <stdarg.h>

#include "kq.h"

FILE* logfp;

void writelog(const char* fmt, ...){
    va_list args;
    logfp = fopen("LOG", "a");

    va_start(args, fmt);
    vfprintf(logfp, fmt, args); 
    va_end(args);
    fclose(logfp);
}

struct window{
    WINDOW* w;
    _Bool pad;
    pthread_mutex_t lock;
    int lines;
    int line_cap;
    int xmax, ymax;
    int scroll_pos;
};

/* used for kq_reader_thread */
struct kq_win_pair{
    struct kq* k;
    struct window* w;
};

void label_box(struct window* w){
    int x, y;
    getyx(w->w, y, x);
    mvwprintw(w->w, 0, 4, "ASHNET WIRELESS COMMUNICATION PROTOCOL");
    wmove(w->w, y, x);
}

void window_refresh(struct window* w){
    int start_ln;
    if(w->pad){
        box(w->w, 0, 0);
        refresh();
        start_ln = (w->lines >= w->ymax) ? w->lines-w->ymax : 0;
        (void)start_ln;
        label_box(w);
        prefresh(w->w, 0, 0, 0, 0, w->lines, w->xmax);
    }
    else wrefresh(w->w);
}

/* this must be threadsafe because the message box is written to both when messages are being sent AND
 * when a message is popped from the kq and displayed to the user */
void append_window(struct window* w, char* str){
    int x, y;
    pthread_mutex_lock(&w->lock);
    getyx(w->w, y, x);
    (void)x;
    /*wmove(w->w, y+1, 1);*/
    wmove(w->w, y, 1);
    wprintw(w->w, "%s", str);
    window_refresh(w);
    ++w->lines;
    pthread_mutex_unlock(&w->lock);
}

void resize_windows(struct window* messages, struct window* text, int input_fraction){
    int x, y, text_lines;
    getmaxyx(stdscr, y, x);
    text_lines = y/input_fraction;
    text_lines = (text_lines < 3) ? 3 : text_lines;
    messages->ymax = y-text_lines;
    messages->xmax = x;
    wresize(messages->w, y-text_lines, x);
    wresize(text->w, text_lines, x);
    /*box(messages->w, 0, 0);*/
    box(text->w, '|', '+');
    window_refresh(messages);
    window_refresh(text);
    refresh();
}

void init_windows(struct window* messages, struct window* text, int input_fraction){
    int x, y, text_lines;

    messages->scroll_pos = 0;
    messages->lines = text->lines = 0;
    messages->w = malloc(sizeof(WINDOW));
    text->w = malloc(sizeof(WINDOW));

    pthread_mutex_init(&messages->lock, NULL);
    cbreak();
    initscr();
    getmaxyx(stdscr, y, x);
    text_lines = y/input_fraction;
    text_lines = (text_lines < 3) ? 3 : text_lines;
    // resizing later anyway
    /*messages->w = newpad(1000, 176);*/
    messages->w = newpad(y-text_lines, x);
    box(messages->w, 0, 0);
    messages->pad = 1;
    /*getmaxyx(messages->w, y, x);*/
    messages->line_cap = 1000;
    messages->ymax = y-text_lines;
    messages->xmax = x;

    text->w = newwin(text_lines, x, y-text_lines, 0);
    text->pad = 0;

    scrollok(messages->w, 1);
    idlok(messages->w, 1);
    keypad(messages->w, 1);

    // how to resize relative to old dims
    /*wresize(messages->w, y-text_lines, x);*/
    /*wresize(messages->w, text_lines, x);*/
    box(text->w, '|', '+');
    refresh();
    /*box(messages->w, 0, 0);*/
    /*waddstr(messages->w, "MESSAGES\n");*/
    /*wprintw(messages->w, "MESSAGES\n");*/
    /*mvwprintw(text->w, 0, 1, "HELLO");*/
    window_refresh(messages);
    /*wrefresh(messages->w);*/
    wrefresh(text->w);
}

char* process_kq_msg(uint8_t* bytes){
    char* cursor = (char*)bytes;
    char* ret;
    strsep(&cursor, ",");
    if(!cursor)return NULL;
    ret = cursor;
    cursor = strchr(cursor, ',');
    if(!cursor)return NULL;
    *cursor = ':';
    /* ret-1 is guaranteed to exist - ',' */
    memmove(ret-1, ret, cursor-ret+1);
    *cursor = ' ';
    return ret-1;
}

void* kq_reader_thread(void* vkw){
    struct kq_win_pair* kw = vkw;
    uint8_t* to_free;
    char* recvd;
    while(1){
        recvd = process_kq_msg((to_free = pop_kq(kw->k, NULL)));
        if(recvd)append_window(kw->w, recvd);
        free(to_free);
    }
}

void broadcast_msg(char* msg, struct window* w, struct kq* k){
    char buf[1000] = {0};
    sprintf(buf, "YOU: %s", msg);
    insert_kq(k, msg, 11);
    append_window(w, buf);
    writelog("append window called with %s from broadcast_msg\n", msg);
}

int main(int a, char** b){
    struct window msg, txt;
    struct kq k;
    char buf[200] = {0};
    int idx = 0, c;
    pthread_t kq_reader_pth;
    struct kq_win_pair kw = {.k=&k, .w=&msg};

    if(a < 3)return 0;

    init_windows(&msg, &txt, 7);
    init_kq(&k, atoi(b[1]), atoi(b[2]));
    pthread_create(&kq_reader_pth, NULL, kq_reader_thread, &kw);

    /* not sure why this is necessary, but without
     * writing whitespace to each line of the message
     * box, the border doesn't get drawn
     */
    for(int i = 0; i < msg.ymax; ++i){
        append_window(&msg, "");
    }
    wmove(msg.w, 1, 1);
    wmove(txt.w, 1, 1);
    refresh();

    /*getchar();*/
    while(1){
        /*switch((c = getchar())){*/
        switch((c = wgetch(msg.w))){
            case KEY_UP:
            case KEY_LEFT:
            case KEY_RIGHT:
                /*for(int i = 100; i > 0; --i)*/
                prefresh(msg.w, 100, 0, 0, 0, msg.ymax, msg.xmax);
                break;
            case 'w':
                resize_windows(&msg, &txt, 5);
                break;
            case KEY_EXIT:
            case 'q':
                goto cleanup;
            case '\r':
            case '\n':
                buf[idx++] = '\n';
                broadcast_msg(buf, &msg, &k);
                /*append_window(&msg, buf);*/
                wmove(txt.w, 1, 1);
                memset(buf, ' ', idx);
                /*werase(txt.w);*/
                /*wrefresh(txt.w);*/
/*
 *                 hmm i'm not able to overwrite text on screen
 *                 need to overwrite it and make room for next str
 * 
 *                 i'll be able to scroll through messages somehow
 *                 and scroll through my sents as well!
 * 
 *                 for(int i = 0; i < idx; ++i)buf[i] = ' ';
*/
                /*wprintw(txt.w, 0, 1, buf);*/
                /*waddstr(txt.w, buf);*/
                wprintw(txt.w, "%s", buf);
                wmove(txt.w, 1, 1);
                memset(buf, 0, idx);
                /*waddstr(txt.w, buf);*/
                /*wrefresh(txt.w);*/
                window_refresh(&txt);
                idx = 0;
                break;
            case 127:
                /*goto cleanup;*/
            default:
                writelog("captured a char: '%c'\n", c);
                waddch(txt.w, c);
                wrefresh(txt.w);
                buf[idx++] = c;
        }
    }
    /*getchar();*/
    cleanup:
    endwin();
    pthread_mutex_destroy(&txt.lock);
    return 0;
}
