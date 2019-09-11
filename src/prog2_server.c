/* Assignment 2
 * Computer Networks, Winter 2019
 * Tanzima Islam
 *
 * Tony Dinh, Anders Bergquist
 *
 */


#include "trie.h"
#include "prog2_server.h"

#define QLEN 6 /* size of request queue */

/*
* Program: prog2_server
* 
* Purpose: Allocates a socket and waits for two clients to connect before
* forking a new process. The two clients will then play the game on the fork and
* the server will go back to waiting for two more processes.
*
* Syntax: ./prog2_server server_port word
*
* server_port - protocol port number to use
* 
* word - secret word client must guess
*
*------------------------------------------------------------------------
*/

int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in cad; /* structure to hold client's address */
	int sd, sd1, sd2; /* socket descriptors */
	int port; /* protocol port number */
	socklen_t alen; /* length of address */
	int optval = 1; /* boolean value when we set socket option */

	if( argc != 5 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server server_port board_size seconds_per_round path_to_dictionary\n");
		exit(EXIT_FAILURE);
	}

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
        
   sad.sin_family = AF_INET;
        
	sad.sin_addr.s_addr = INADDR_ANY;
        
	port = atoi(argv[1]); /* convert argument to binary */
	if (port > 0 && port > 1023 && port < 65535) { /* test for illegal value */
                sad.sin_port = htons(port);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}
        
        sd = socket(AF_INET, SOCK_STREAM, ptrp->p_proto);
        
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Allow reuse of port - avoid "Bind failed" issues */
	if( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	if (bind(sd, (struct sockaddr *) &sad, sizeof(sad)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	if (listen(sd, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}
  
   uint8_t boardSize = atoi(argv[2]);
   if(boardSize > 255) {
      fprintf(stderr, "Error: Board size too large\n");
      exit(EXIT_FAILURE);
   }

   char board[boardSize];
   memset(board, 0, boardSize);
   uint8_t seconds = atoi(argv[3]);
   if(seconds > 255 || seconds == 0) { 
      fprintf(stderr, "Error: Seconds too large\n");
      exit(EXIT_FAILURE);
   }

   char player;
   
   int fd;
   char* dictionary = argv[4];

   fd = open(dictionary, O_RDONLY, 0777);
   if(fd == -1) {
      fprintf(stderr, "Error: Open failed\n");
      exit(EXIT_FAILURE);
   }
   
   struct TrieNode* root = getNode(); 

   trie_init(root, fd); 

	/* Main server loop - accept and handle requests */
	while (1) {
      player = '1';
		alen = sizeof(cad);
		if ( (sd1=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}

      send_board_data(sd1, &player, &boardSize, &seconds);


      if ( (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
	      fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}

      player = '2';
      send_board_data(sd2, &player, &boardSize, &seconds);

      const pid_t cpid = fork();

      switch(cpid) {

         case -1: {
            perror("fork");
            break;
         }

         case 0: {

            uint8_t round = 1;
            uint8_t scorep1 = 0;
            uint8_t scorep2 = 0;
            
            srand(time(NULL));
            generate_board(board, boardSize);
            
            send_round_info(sd1, &round, &scorep1, &scorep2);
            send_round_info(sd2, &round, &scorep1, &scorep2);
            send(sd1, board, boardSize, 0);
            send(sd2, board, boardSize, 0);
            
            while(scorep1 < 3 && scorep2 < 3) {
                            
               if(round % 2 == 0) {
                  play(sd2, sd1, root, board, boardSize, &scorep2, &scorep1);
               }

               else {
                  play(sd1, sd2, root, board, boardSize, &scorep1, &scorep2); 
               }
               
               generate_board(board, boardSize);
               
               send_round_info(sd1, &round, &scorep1, &scorep2);
               send_round_info(sd2, &round, &scorep1, &scorep2);
               send(sd1, board, boardSize, 0);
               send(sd2, board, boardSize, 0);
               
               round++;
            }

            close_clients(sd1, sd2);
            break;
         }
         
         default: {
            close(sd1);
            close(sd2);
            break;
         }
      }
   }
}

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
void trie_init(struct TrieNode* root, int fd) {

   int maxLineLength = 256;
   char dictWord[maxLineLength];
   int i;
   int moreWords = 1;

   while(moreWords) {

      for(i = 0; i < maxLineLength; i++) {
         read(fd, &dictWord[i], sizeof(char));
    
         if (dictWord[i] == EOF) {
            moreWords = 0;
            break;
         }
         if (dictWord[i] == '\n') {
            break;
         }
      }
      dictWord[i] = '\0';
      insert(root, dictWord);
   }
}

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
uint8_t isValidWord(char* word, char* board, uint8_t boardSize, struct TrieNode* root) {
    
    uint8_t correctWord = 1;
    int i;
    
    char* boardCopy = strdup(board);
    char* charLoc;
    
    boardCopy[boardSize] = '\0';
    
    if (!search(root, word)) {
        correctWord = 0;
        return correctWord;
    }
    
    for(i = 0; i < strlen(word); i++) {
      charLoc = strchr(boardCopy, word[i]);
      
      if (charLoc == NULL) {
          correctWord = 0;
          return correctWord;
      }
      else {
          *charLoc = '1';
      }
    }
    return correctWord;
}

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
int checkList(linkedList* list, char* word) {
    
  int inList = 0;
  while(list != NULL && strcmp(list->word, word) != 0) {
     list = list->next;
  }

  if(list != NULL) {
     inList = 1;
  }

  return inList;
}

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
void append(linkedList** head, char* word) {

    linkedList* newNode = (linkedList*) malloc(sizeof(linkedList));     
    linkedList* lastNode = *head;
    
    char* nw = strdup(word);

    newNode->word = nw;     
    newNode->next = NULL;     
    
    if(*head == NULL) {         
       *head = newNode;
    }
    
    else {         
       while(lastNode->next != NULL) {             
          lastNode = lastNode->next;  
       }
       lastNode->next = newNode;     
    }
}

/* Free Linked List
 *
 * void
 *
 * head  list to be freed
 *
 * Frees all allocated memory for the linked list
 *
 */
void free_linked_list(linkedList* n) {
    
    while(n != NULL) {
        linkedList* curr = n;
        n = n->next;
        free(curr);
    }

}

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
void generate_board(char* board, uint8_t boardSize) {
   
   char rand;
   int rn;
   int has_vowels = 0;
   char* vowels = "aeiou";

   for(int i = 0; i < boardSize; i++) {
      
      rand = 'a' + (random() % 26);
      board[i] = rand;

      if(strchr(vowels, rand) != NULL)
         has_vowels = 1;
      
   }

   if(has_vowels == 0) {
      rn = (random() % 5);
      board[boardSize - 1] = vowels[rn];
   }

}

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
         char* board, uint8_t boardSize, uint8_t* s1, uint8_t* s2) {

   int sd_temp;
   int turn = 1;
   char y = 'Y';
   char n = 'N';
   uint8_t valid = 1; 

   linkedList* head = NULL;
   
   while(valid) {
      char word[256];
      uint8_t word_len;
      
      send(sd_other, &n, sizeof(char), 0);
      send(sd_curr, &y, sizeof(char), 0);
      
      
      detect_close(recv(sd_curr, &word_len, sizeof(uint8_t), 0), sd_curr, sd_other);
                  
      if (word_len == 0) {
         send(sd_curr, &word_len, sizeof(uint8_t), 0);
         send(sd_other, &word_len, sizeof(uint8_t), 0);
         valid = 0;
      }
                  
      else {
         detect_close(recv(sd_curr, word, word_len, 0), sd_curr, sd_other);
         
         word[--word_len] = '\0';
                  
         valid = isValidWord(word, board, boardSize, root);
                  
         if (valid) {
            if (checkList(head, word)) {
               valid = 0;
               send(sd_curr, &valid, sizeof(uint8_t), 0);
               send(sd_other, &valid, sizeof(uint8_t), 0);   
            }
            else {
              
               append(&head, word);

               send(sd_curr, &valid, sizeof(uint8_t), 0);
               send(sd_other, &word_len, sizeof(uint8_t), 0);
               send(sd_other, word, word_len, 0);
            }
         }
                  
         else {
            send(sd_curr, &valid, sizeof(uint8_t), 0);
            send(sd_other, &valid, sizeof(uint8_t), 0);
         }
      }
      sd_temp = sd_curr;
      sd_curr = sd_other;
      sd_other = sd_temp;
      turn++;
   }

   if(turn % 2 != 0) {
      *s1 = *s1 + 1;
   }
   else {
      *s2 = *s2 + 1;
   }

   free_linked_list(head);
}

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
int send_board_data(int sd, char* player, uint8_t* bs, uint8_t* secs) {
   
   int n = 0;

   n += send(sd, player, sizeof(char), 0);
   n += send(sd, bs, sizeof(uint8_t), 0);
   n += send(sd, secs, sizeof(uint8_t), 0);
   
   return n;
}

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
int send_round_info(int sd, uint8_t* round, uint8_t* scorep1, uint8_t* scorep2) {

   int n = 0;

   n += send(sd, round, sizeof(uint8_t), 0);
   n += send(sd, scorep1, sizeof(uint8_t), 0);
   n += send(sd, scorep2, sizeof(uint8_t), 0);
   
   return n;
}

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
void detect_close(int numbytes, int sd1, int sd2) {
    if (numbytes == 0) {
        close_clients(sd1, sd2);
    }
}

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
void close_clients(int sd1, int sd2) {
    close(sd1);
    close(sd2);
    exit(1);
}


/*
static void print_list(linkedList* n) {

//     printf("print_list\n");
    while (n != NULL) {
//         printf("word: %s\n", n->word);
        n = n->next;
        printf("\n");
    }
//     printf("print_list after\n");
}
*/





