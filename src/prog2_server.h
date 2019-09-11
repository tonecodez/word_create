/* Header file for assignment 2
 *
 */

#ifndef prog2_server_h
#define prog2_server_h

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h> 
#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

typedef struct linkedList linkedList;

struct linkedList {
    linkedList* next;
    char* word;
};

// Start trie functions

/* Initiate trie
 *
 * void
 *
 * root     node that indicates the root of the trie
 * fd       file descriptor containing file to import
 *
 * Populates the a trie structure
 *
 */
void trie_init(struct TrieNode* root, int fd);

/* Is Valid Word
 *
 * uint8_t     0 if invalid word, 1 if valid
 *
 * word        word to be checked for
 * board       board containing the available letters
 * boardSize   size of board
 * root        trie node root
 *
 * Checks for 2 things
 *    1. If the word is in the trie
 *    2. If the characters of the word are contained on the board
 *
 */
uint8_t isValidWord(char* word, char* board, uint8_t boardSize, struct TrieNode* root);

// End trie functions
// -----------------------------------------------------------------------
// Start linked list functions

/* Check List
 *
 * int   returns 1 if in list, 0 if not in list
 *
 * list  list to traverse
 * word  word to compare
 *
 * Traverses through the linked list and checks if the word is contained within
 * any of the nodes on the list.
 *
 */
int checkList(linkedList* list, char* word);

/* Append
 *
 * void
 *
 * head  node to append
 * word  word to add to appended node
 *
 * Appends to linked list. If it is the first node in the list, the node will be
 * considered the head node of the list.
 *
 */
void append(linkedList** head, char* word);

/* Free Linked List
 *
 * void
 *
 * head  list to be freed
 *
 * Frees all allocated memory for the linked list
 *
 */
void free_linked_list(linkedList* head);

// End linked list functions
// -----------------------------------------------------------------------
// Start game logic functions

/* Generate board
 *
 * void
 *
 * board       board to be populated
 * boardSize   given board size through command line
 *
 * Generates a random board with length given by user through command line
 *
 */
void generate_board(char* board, uint8_t boardSize);

/* Play game
 *
 * void
 *
 * sd_curr     current player whos turn it is
 * sd_other    other player
 * root        trie node root
 * board       board to be sent to client
 * boardSize   size of board
 * s1          score of player 1
 * s2          score of player 2
 *
 * Goes through general gameplay, sends all necessary info for user to play the
 * game. This function tells the clients if their word was correct, incorrect,
 * and tells the clients whose turn it is.
 *
 */
void play(int sd_curr, int sd_other, struct TrieNode* root, 
        char* board, uint8_t boardSize, uint8_t* s1, uint8_t* s2);

/* Send Board Data
 *
 * int   bytes sent
 *
 * sd       socket descriptor of client
 * player   char indicated which player 
 * bs       board size
 * secs     number of seconds to play
 *
 * Tells client which player they are, the board size and how many seconds they
 * have to guess a word.
 *
 */
int send_board_data(int sd, char* player, uint8_t* bs, uint8_t* secs);

/* Send round info
 *
 * int   bytes sent
 *
 * sd       socket descriptor of current player
 * round    current round of the game
 * scorep1  score of player 1
 * scorep2  score of player 2
 *
 * Sends info after each round
 *
 *
 */
int send_round_info(int sd, uint8_t* round, uint8_t* scorep1, uint8_t* scorep2);

/* Detect close
 *
 * void
 *
 * numbytes    number of bytes received
 * sd1         sd of curr player
 * sd2         sd of other player
 *
 * Calls close_clients if numbytes == 0
 * 
 */
void detect_close(int numbytes, int sd1, int sd2);

/* Close Clients
 *
 * void 
 *
 * sd1   sd of curr player
 * sd2   sd of other player
 *
 * Closes the clients, exits the process
 *
 *
 */
void close_clients(int sd1, int sd2);

// End game logic functions

#endif
