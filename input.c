#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <curses.h>


typedef struct ThreadInput{
    pthread_mutex_t mutex;
    char *perm_char;
} ThreadInput;


void userInput (ThreadInput inp);

int main (int argc, char *argv[]) {


    // first begin by establishing the curses stuff
    initscr();
    cbreak();
    curs_set(0);

    // tracks most recent valid input
    char input_char = 'z';


    // prepare thread stuff
    pthread_t input_thread;                                     
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 
    ThreadInput stuff;
    stuff.mutex = mutex;
    stuff.perm_char = &input_char;
    // create thread
    pthread_create(&input_thread, NULL, (void *)&userInput, (void *)&stuff);

    while (1) {
        pthread_mutex_lock(&mutex);
        //printf("human input: %c\n", input_char);
        waddch(stdscr, input_char);
        wrefresh(stdscr);
        pthread_mutex_unlock(&mutex);
        sleep(1);
        if (input_char == 'd') break;
    } 
    endwin(); 
}


// to be used with a thread. Collects input from user.
// should also sanitize/allow only the direction keys
void userInput (ThreadInput input) {
   
    char temp_ch; 

    while (1) {
        if ( (temp_ch = getch()) != ERR) {
            if (temp_ch == 'w' || temp_ch == 'a' || temp_ch == 's' || temp_ch == 'd') {
                pthread_mutex_lock(&(input.mutex));
                printf("new character: %c\n", temp_ch);
                *(input.perm_char) = temp_ch;
                pthread_mutex_unlock(&(input.mutex));
            }
        }
    }
}
