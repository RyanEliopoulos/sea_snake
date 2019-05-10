/*
 *  BUGS: I think the mutex is being destoryed in the stack frame calling pthread_create, causing error when other thread attempts to lock
 *
 *  -----Design decisions
 *
 *  Hitting the wall kills the snake.
 *  Hitting itself  kills the snake.
 *  Eating food causes the snake to grow by 1 char in size. New piece appears at the square the food was at once entire snake traverses the spot.
 *
 *  The snake will be a linked list
 *  Non-snake items on the map will be contained in a single linked list
 *
 *  spawning new items check the item and snake lists searching for open spaces
 *  
 *
 *
 *  ----Functions
 *
 *  paint(): To draw the contents of the screen (can probably exclude the border)
 *  wash()  Remove all of the characters from the screen (probably going to replace the painted chars with a space (blank))
 *
 */



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

// initialize program state
void init (ThreadInput *);

// meant to be its own thread. Updates current char variable in main thread
void userInput (ThreadInput *);

// encloses the terminal with 
// the given height, width
void printBorder(int, int);

int main (int argc, char *argv[]) {

    // tracks most recent valid input 
    char input_char = 'w';  // defaults to moving north

    // initing means of inter-thread communication
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 
    ThreadInput comm_data;
    comm_data.mutex = mutex;
    comm_data.perm_char = &input_char;
    
    // estalishing initial game state.
    // Launches input thread
    init (&comm_data);


    while (1) {
        pthread_mutex_lock(&mutex);
        //waddch(stdscr, input_char);
        //wrefresh(stdscr);

        //  wash(snake, items)                  // replace all map symbols with a blank
        //  slither(snake, items, input_char)   // movement calulations. Updates game state
        //  evalState(snake, items)             // check if game has reached and end state. Exit if so.
        //  spawnItems(snake, items)            // spawn food for the snake
        //  paint(snake, items)                 // draw the new game state
        
        waddch(stdscr, input_char);
        pthread_mutex_unlock(&mutex);
        sleep(1);
        if (input_char == 'd') break;
    } 


    endwin(); 
    exit(0);
}

void init (ThreadInput *comm_data) {

    // first begin by establishing the curses stuff
    initscr();
    cbreak();   // no buffering user input
    noecho();
    curs_set(0);  // make cursor invisible

    // prepare screen 
    int height, width;
    getmaxyx(stdscr, height, width); 
    printBorder(height, width); 

    // prepare thread stuff
    pthread_t input_thread;                                     
    // create thread
    wmove(stdscr, 10, 10);
    //waddch(stdscr, '8');
    pthread_create(&input_thread, NULL, (void *)&userInput, (void *)comm_data);
}

// to be used with a thread. Collects input from user.
// only whitelisted keystrokes are passed to the main thread
void userInput (ThreadInput *input) {

    char temp_ch; 
    while (1) {
        if ( (temp_ch = getch()) != ERR) {
            if (temp_ch == 'w' || temp_ch == 'a' || temp_ch == 's' || temp_ch == 'd') {
                pthread_mutex_lock(&(input->mutex));    // program breaks
                *(input->perm_char) = temp_ch;        // program break
                pthread_mutex_unlock(&(input->mutex));
            }
        }
    }
}

// height and width of the terminal
void printBorder(int h, int w) {
    
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            wmove(stdscr, i, j);
            if (i == 0 || i == (h-1)) {         // printing the top and bottom
                waddch(stdscr, '_');
            }
            else if (j == 0 || j == (w-1)) {    // printing the left and right
                waddch(stdscr, '|');
            }
        }
    }
 

}
