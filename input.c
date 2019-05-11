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
 *  evalState is not complete. Need to check if snake hits itself
 *  slither is not complete. Still need eating logic. OK it eats but isn't growing
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
    

// as titled
void spawnItems (struct SnakeNode *, struct ItemNode **);

// check for temrinal game condition
void evalState (struct SnakeNode *);

// updates snake and item values
void slither (struct SnakeNode *, struct ItemNode **, char);

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

    // sleep timer
    struct timespec nap;
    nap.tv_sec = 0;
    nap.tv_nsec = 90000000;
        
    
    while (1) {
        pthread_mutex_lock(&mutex);
        wash(snake, items);                     // replace all map symbols with a blank (' ')
        slither (snake, &items, input_char);     // movement calculations. Updates game state
        evalState(snake);                       // check if game has reached and end state. Exit if so.
        spawnItems(snake, &items);               // spawn food for the snake
        paint(snake, items);                    // draw new game state to terminal
        pthread_mutex_unlock(&mutex);
        if (input_char == 'q') break;
        //sleep(1);
        nanosleep(&nap, NULL);
        //printf("heartbeat\n");
    } 
    endwin(); 
    exit(0);
}


void evalState (struct SnakeNode *snake) {

    // check if snake hit the edge of the map
    // if either coord is 0: dead
    // if h = max h: dead
    // if w = max w: dead
    if (snake->h == 0 || snake->w == 0) {
        printf("you lose!\n");
        sleep(1);
        endwin();
        exit(0);
    }
    int mxheight, mxwidth;
    getmaxyx(stdscr, mxheight, mxwidth); 
    if (snake->h == mxheight || snake->w == mxwidth) {
        printf(" you are SOL!\n"); 
        sleep(1);
        endwin();
        exit(0);
    }

    // then check if snake hit itself
}


void spawnItems (struct SnakeNode *snake, struct ItemNode **items) {

    // roll spawn chance
    if ((rand() % 4) == 1) {        // aiming for 25% chance of spawning item per tick.
        //printf("item should be spawning\n");
        // coord vars
        int mxh, mxw;               
        getmaxyx(stdscr, mxh, mxw);
        int h, w;                   // hold new coordinates

        while (1) {                 // loop until an available space is found
            h = rand() % mxh;
            w = rand() % mxw;
            int fail = 0;
            // first check the snake nodes   
            struct SnakeNode *temp_s = snake;
            while (temp_s!= NULL) {
                if (temp_s->h == h && temp_s->w == w) {
                    fail = 1;
                    break;
                }
                temp_s = temp_s->next_node;
            }
            if (fail) continue;
            // now check item list
            struct ItemNode *temp_i = *items;
            while (temp_i != NULL) {
                //printf("stuck here\n");
                if (temp_i->h == h && temp_i->w == w) {
                    fail = 1;
                    break;
                }
                temp_i = temp_i->next_node;
            }
            if (!fail) break;
        }
        //printf("new h: %d\n new w: %d\n", h, w);
        // Now have a valid h, w pair.
        // append the item to the list
        struct ItemNode *newnode = malloc(sizeof(struct ItemNode));
        newnode->h = h;
        newnode->w = w;
        newnode->next_node = NULL;

        if (*items == NULL) {                // list is already empty    
            //printf("yes, here indeed\n");
            *items = newnode; 
        }
        else {
            struct ItemNode *temp = *items;
            while (temp->next_node != NULL) {         // find end of the list
                temp = temp->next_node;
            }
            temp->next_node = newnode;
        }
    }
}

void slither (struct SnakeNode *snake, struct ItemNode **items, char direction) {

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
    int old_h, old_w, old_growth;
    int next_growth = 0;                // if 8 this tick, handled later in this fnx 
    struct SnakeNode *currnode = snake;

    while (currnode->next_node != NULL) {
        // save snake coords before updating
        old_h = currnode->h;
        old_w = currnode->w;
        old_growth = currnode->growth;
        if (old_growth == 1) {
            //printf("growth at: %d, %d\n", old_h, old_w);
        }
        // now update the snake node
        currnode->h = next_h;
        currnode->w = next_w;
        currnode->growth = next_growth;
        currnode = currnode->next_node;
        // and what was once old is now new
        next_h = old_h; 
        next_w = old_w;
        next_growth = old_growth;
    }
    
    currnode->h = next_h;
    currnode->w = next_w;
    currnode->growth = next_growth;
     
    
    // base case: tail node
    if (currnode->growth == 1) {     // transfer old tail node data to new before updating coordinates
        //printf("grdfsefsefsefowth node\n");
        currnode->growth = 0;
        old_h = currnode->h;
        old_w = currnode->w;
        // create the new tail
        struct SnakeNode *temp = malloc(sizeof(struct SnakeNode *));
        temp->h = old_h;
        temp->w = old_w;
        temp->next_node = NULL;
        temp->growth = 0;        
        currnode->next_node = temp;

    }
    
    // update coordinates of former tail
    currnode->h = next_h;
    currnode->w = next_w;


    // now check if the snake has eaten anything
    //  if (snake->h, snake->w coord pair in [items]
    //  currnode->growth = 1;
    //  //cut item from the items list
    //  free(consumed_item)
   
    // deal with case: items is only one node 
    // this may actually be unnecessary
    //printf("this far? 00\n");
    if (*items == NULL) {
        return;
    }

    if ((*items)->next_node == NULL) {
        //printf("found it, eh?\n");
        if ((*items)->h == snake->h && (*items)->w == snake->w) {
            free(*items);
            *items = NULL;
            return;
        }
    }

    //printf("this far? 11\n");
    struct ItemNode *previtem, *curritem;
    previtem = NULL; curritem = *items;
    while (curritem != NULL) {
        //printf("this far?\n");
        if (curritem->h == snake->h && curritem->w == snake->w) {   // snake did eat.
            snake->growth = 1;
            //printf("snake did grow. Wohoo\n");
            if (previtem == NULL) {                                 // The first node is the one being deleted
                *items = curritem->next_node;
                free(curritem);
                return;
            }
            else {
                previtem->next_node = curritem->next_node;
                free(curritem); 
                return;
            }
        }
        previtem = curritem;
        curritem = curritem->next_node;

    }
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
            if (temp_ch == 'w' || temp_ch == 'a' || temp_ch == 's' || temp_ch == 'd' || temp_ch == 'q') {
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
