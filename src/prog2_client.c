/* Assignment 2
 * Computer Networks, Winter 2019
 * Tanzima Islam
 *
 * Tony Dinh, Anders Bergquist
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/*------------------------------------------------------------------------
* Program: prog2_client
*
* Purpose: Allocate a socket, connect to a server, if it is an number person to
* connection, it will play as player 1, otherwise it will play as player 2.
* Sends a word to the server.
*
* Syntax: ./prog2_client server_address server_port
*
* server_address - name of a computer on which server is executing
* server_port    - protocol port number server is using
*
*------------------------------------------------------------------------
*/

int main(int argc, char **argv) {
	struct hostent *ptrh; /* pointer to a host table entry */
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold an IP address */
	int sd; /* socket descriptor */
	int port; /* protocol port number */
	char *host; /* pointer to host name */

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./client server_address server_port\n");
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[2]); /* convert to binary */
	if (port > 0 && port > 1023 && port < 65535) /* test for legal value */
		sad.sin_port = htons((u_short)port);
	else {
		fprintf(stderr,"Error: bad port number %s\n",argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1]; /* if host argument specified */

	/* Convert host name to equivalent IP address and copy to sad. */
	ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		fprintf(stderr,"Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	/* Map TCP transport protocol name to protocol number. */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	if (connect(sd, (struct sockaddr *) &sad, sizeof(sad)) < 0) {
		fprintf(stderr,"connect failed\n");
		exit(EXIT_FAILURE);
	}

   char player;
   uint8_t boardSize;
   uint8_t seconds;

   recv(sd, &player, sizeof(char), 0);
   recv(sd, &boardSize, sizeof(uint8_t), 0);  
   recv(sd, &seconds, sizeof(uint8_t), 0);
      
   printf("You are Player %c...", player);

   if(player == '1') {
      printf("The game will begin when Player 2 joins...\n");
   }

   printf("\nBoard size: %d\nSeconds per turn: %d\n\n", boardSize, seconds);

   uint8_t round;
   uint8_t scorep1 = 0;
   uint8_t scorep2 = 0;
   char board[boardSize + 1];
   char word[256];
   int still_playing = 1;

   int ready_reading;
   fd_set input_set;
   struct timeval timeout;
   FD_ZERO(&input_set);
   FD_SET(0, &input_set);

   
   uint8_t valid;
   int nbr;
   
   nbr = recv(sd, &round, sizeof(uint8_t), 0);
   nbr = recv(sd, &scorep1, sizeof(uint8_t), 0);
   nbr = recv(sd, &scorep2, sizeof(uint8_t), 0);
   nbr = recv(sd, board, boardSize, 0);
    
   while(scorep1 < 3 && scorep2 < 3 && nbr > 0) {

      memset(word, 0, 256);
      
      board[boardSize] = '\0';

      printf("Round %d...\n", round);
      printf("Score is %d-%d\n", scorep1, scorep2);
      
      printf("Board: ");

      for(int i = 0; i < boardSize; i++) {
         printf("%c",board[i]);
         
         if(i != boardSize - 1)
            printf(" ");
      }
      printf("\n");
      
      still_playing = 1;
      
      while(still_playing && nbr > 0) {
          
         memset(word, 0, 256);
         uint8_t word_len;
         char turn;
         uint8_t n = 0;
         
         nbr = recv(sd, &turn, sizeof(char), 0);
         
         if(turn == 'Y') { 

            ready_reading = 0;
            timeout.tv_sec = seconds;
            timeout.tv_usec = 0;
            printf("Your turn, enter word: ");
            fflush(stdout);
            ready_reading = select(1, &input_set, NULL, NULL, &timeout);

            if(ready_reading == -1) {
               
               perror("select");
               return -1;

            }
            
            if(ready_reading) {
               
               n = read(0, word, 256);

            }
            
            word_len = n;
            send(sd, &word_len, sizeof(uint8_t), 0);
            if(word_len == 0) {           
               printf("Timed out!\n\n");
            }

            else {
               send(sd, word, word_len, 0);
            }

            nbr = recv(sd, &valid, sizeof(uint8_t), 0);
            if(!valid) {
                printf("Invalid word!\n\n");
                still_playing = 0;
            }
         }

         else if(turn == 'N') {
            
            printf("Please wait for opponent to enter word...\n");
        
            nbr = recv(sd, &word_len, sizeof(uint8_t), 0);

            if(nbr == 0) {
               break;
            }
            
            if (word_len > 0) {
                nbr = recv(sd, word, word_len, 0);
                word[word_len] = '\0';
                printf("Opponent entered: \"%s\"\n", word);
            }
            else {
                printf("Opponent lost the round!\n\n");
                still_playing = 0;
            }
         }
         
      }
      
        nbr = recv(sd, &round, sizeof(uint8_t), 0);
        nbr = recv(sd, &scorep1, sizeof(uint8_t), 0);
        nbr = recv(sd, &scorep2, sizeof(uint8_t), 0);
        nbr = recv(sd, board, boardSize, 0);
   }
   
   if(scorep1 > scorep2) {
        if(player == '1')
            printf("You won!\n");
        
        else {
            printf("You lost!\n");
        }
   }
   
   else if (scorep2 > scorep1) {
        if(player == '2')
            printf("You won!\n");
        
        else {
            printf("You lost!\n");
        }
   }
   

    close(sd);
    exit(EXIT_SUCCESS);
}


