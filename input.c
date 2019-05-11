/*
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


// shared variables between threads
typedef struct ThreadInput {
    pthread_mutex_t mutex;
    char *perm_char;
} ThreadInput;


// track snake parts on the grid
typedef struct SnakeNode {
    int h;
    int w;
    struct SnakeNode* next_node;
    int growth;
} SnakeNode;

// track food positions on the grid
typedef struct ItemNode {
    int h;
    int w;
    struct ItemNode* next_node;
} ItemNode;
    

// updates snake and item values
void slither (struct SnakeNode *, struct ItemNode *, char);

// clear the screen
void wash (struct SnakeNode *, struct ItemNode *);

// populate screen with pixels, yo
void paint (struct SnakeNode *, struct ItemNode *);

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
    
    // Launches input thread
    init (&comm_data);

    // init snake
    struct SnakeNode *snake = malloc (sizeof(struct SnakeNode));
    snake->h = 10;
    snake->w = 10; 
    snake->next_node = malloc (sizeof(struct SnakeNode));
    snake->next_node->h = 11;
    snake->next_node->w = 10;
    snake->next_node->next_node = NULL;

    // init empty item list
    struct ItemNode *items = NULL;

    while (1) {
        wash(snake, items);
        pthread_mutex_lock(&mutex);
        slither (snake, items, input_char);
        //  wash(snake, items)                  // replace all map symbols with a blank
        //  slither(snake, items, input_char)   // movement calulations. Updates game state
        //  evalState(snake, items)             // check if game has reached and end state. Exit if so.
        //  spawnItems(snake, items)            // spawn food for the snake
        //  paint(snake, items)                 // draw the new game state
        
        pthread_mutex_unlock(&mutex);
        sleep(4);
        paint(snake, items);
        if (input_char == 'd') break;
        sleep(2);
    } 
    endwin(); 
    exit(0);
}


void slither (struct SnakeNode *snake, struct ItemNode *items, char direction) {

    /*** first determine the next coordinates for the snake's head ****/
    int next_h = snake->h;
    int next_w = snake->w;

    if (direction == 'w') {
        next_h = snake->h - 1;
    }
    else if (direction == 'a') {
        next_w = snake->w - 1;
    }
    else if (direction == 's') {
        next_h = snake->h + 1;
    }
    else if (direction == 'd') {
        next_w = snake->w + 1;
    }
   
    // propogate coordinates down the snake
    int old_h, old_w;       
    struct SnakeNode *currnode = snake;

    while (currnode->next_node != NULL) {
        // save snake coords before updating
        old_h = currnode->h;
        old_w = currnode->w;
        // now update the snake node
        currnode->h = next_h;
        currnode->w = next_w;
        currnode = currnode->next_node;
        // and what was once old is now new
        next_h = old_h; 
        next_w = old_w;
    }

    // base case: tail node
    if (currnode->growth) {     // transfer old tail node data to new before updating coordinates
        currnode->growth = 0;
        old_h = currnode->h;
        old_w = currnode->w;
        // create the new tail
        struct SnakeNode *temp = malloc(sizeof(struct SnakeNode *));
        temp->h = old_h;
        temp->w = old_w;
        temp->next_node = NULL;
        temp->growth = 0;        

    }
    
    // update coordinates of former tail
    currnode->h = next_h;
    currnode->w = next_w;
}

void wash (struct SnakeNode *snake, struct ItemNode *items) {

    // remove the snake 
    while (snake != NULL) {
        wmove(stdscr, snake->h, snake->w);
        waddch(stdscr, ' ');
        snake = snake->next_node;
    }  
    
    while (items != NULL) {
        wmove(stdscr, items->h, items->w);
        waddch(stdscr, ' ');
        items = items->next_node;
    }
    wrefresh(stdscr);
}


void paint (struct SnakeNode *snake, struct ItemNode *items) {


    /***** Snake first  *****/
    // first paint the snake
    // head is different char than the body
    wmove(stdscr, snake->h, snake->w);  // print the head
    waddch(stdscr, '@');

    // traverse the snake's list, printing as we go
    snake = snake->next_node;
    while (snake != NULL) {
        wmove(stdscr, snake->h, snake->w);
        waddch(stdscr, 'O');
        snake = snake->next_node;
    }


    /****  Now items  *****/
    while (items != NULL) {
        wmove(stdscr, items->h, items->w);
        waddch(stdscr, '8');
        items = items->next_node;
    } 
    wrefresh(stdscr);
}
    


// initializes game state and launches child thread.
// not responsible for snake or item initialization
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
    pthread_create(&input_thread, NULL, (void *)&userInput, (void *)comm_data);
}

// to be used with a thread. Collects input from user.
// only whitelisted keystrokes are passed to the main thread
void userInput (ThreadInput *input) {

    char temp_ch; 
    while (1) {
        if ( (temp_ch = getch()) != ERR) {
            if (temp_ch == 'w' || temp_ch == 'a' || temp_ch == 's' || temp_ch == 'd') {
                pthread_mutex_lock(&(input->mutex));    
                *(input->perm_char) = temp_ch;        
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
