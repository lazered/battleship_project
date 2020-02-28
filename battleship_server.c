//Code by Brandon Barrantes for CSI 333 Programming at the Hardware/Software Interface
//FOR SERVER: Compile and input '7894' as the command line argument
//FOR CLIENT: Compile and input the IP that comes up when you run the server and then the same port number '7894'

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <unistd.h>

//BOARD DIMENSIONS
#define ROWS 10
#define COLUMNS 10

//SHIP SIZES
#define CARRIER 5
#define BATTLESHIP 4
#define CRUISER 3
#define SUBMARINE 3
#define DESTROYER 2

//DEFINE PORT NUMBER
#define PORT 7894
#define MAXDATASIZE 100

//GLOBAL FOR-LOOP VARIABLES
int i;
int j;

enum shipOrient{
        HORIZ,
        VERT,
}
DIR;

char miss[10] = "Miss";

int shipSize[] = {CARRIER, BATTLESHIP, CRUISER, SUBMARINE, DESTROYER};
char shipLetter[] ={'C', 'B', 'c', 'S', 'D'}; // lowercase c for cruiser
char shipNames[5][15] = {"Carrier", "Battleship", "Cruiser", "Submarine", "Destroyer"};
char moveStatus[2][6] = {"Miss", "Hit"};

bool alreadyDown[5] = {false, false, false, false, false};
int shipsDown = 0;

bool gameOver = false;

/////////////////////////////////////////////////////

char board[ROWS][COLUMNS];
char **sb; // double pointer sb = ship board (hidden from user)

typedef struct logfile logfile;

struct logfile {
    char row;
    int column;
    char shipType[100];
    bool hitmormiss; // miss = 0 // hit = 1

    logfile * next;
};

void addLog_hit(logfile** head_ref, char *row, int col, char* ship, bool zero_one)
{ 
    logfile *new_log = (logfile*)malloc(sizeof(logfile)); 
  
    logfile *last = *head_ref;  
   
    new_log->row = *row; 
    new_log->column = col;
    strcpy(new_log->shipType, ship);
    new_log->hitmormiss = 1;

    new_log->next = NULL;
  
    if (*head_ref == NULL) 
    { 
       *head_ref = new_log; 
       return; 
    }   
       
    while (last->next != NULL) 
        last = last->next;

    last->next = new_log;
    return;
}

void addLog_miss(logfile** head_ref, char *row, int col, bool zero_one)
{ 
    logfile *new_log = (logfile*)malloc(sizeof(logfile)); 
  
    logfile *last = *head_ref;  
   
    new_log->row = *row; 
    new_log->column = col;
    new_log->hitmormiss = 0;

    new_log->next = NULL;
  
    if (*head_ref == NULL) 
    { 
       *head_ref = new_log; 
       return; 
    }   
       
    while (last->next != NULL) 
        last = last->next;

    last->next = new_log;
    return;
}

void printLog(logfile *head_ref) 
{ 
        while (head_ref != NULL) 
        { 
                printf("\nMove : ");
                printf("%c", head_ref->row); 
                printf("%d\n", head_ref->column);

                if (!(head_ref->hitmormiss))
                {
                        printf("Miss\n");
                }

                if (head_ref->hitmormiss)
                {
                printf("%s Hit!\n", head_ref->shipType);
                }

                head_ref = head_ref->next; 
        }
}

void deleteLog(logfile **head_ref) 
{  
    logfile *current, *next;
    current = *head_ref;
  
    while (current != NULL) 
    { 
        next = current->next;
        free(current);
        current = next;
    }
  
    *head_ref = NULL;

    printf("\nMemory Cleared");
}

void generateBoard()
{

    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLUMNS; j++)
            {
                board[i][j] = '~';
                printf(" ");
                printf("%c", board[i][j]);
            }
        printf("\n");
    }

}

void addShips()
{
        for (int ship = 0; ship <5; ship++)
{
        int direction;
        int rand_i, rand_j;

        bool added = false;

        while(!added) 
        {
        // SETTING UP SHIP PLACEMENT
                rand_i = rand() % 10;
                rand_j = rand () % 10;
                direction  = rand() % 2;

        // CHECKING DIRECTION
                if (direction == 0)
                {
                        DIR = HORIZ;
                }
                else
                {
                        DIR = VERT;
                }

        // HORIZONTAL PLACEMENT --------------------------
                if (DIR == HORIZ)
                {
                // CHECKING FOR SPACE
                    bool spaceAvail = true;
                    for(int i = 0; i < shipSize[ship]; i++)
                    {
                        if(rand_j + i >= COLUMNS)
                        {
                            spaceAvail = false;
                            break;
                        }

                        if(     (*(*(sb+rand_i)+rand_j+i) != '~')    )
                        {
                            spaceAvail = false;
                            break;
                        }
                    }
                    if(!spaceAvail) {
                        // IF NO SPACE, RESTART LOOP
                        continue;
                    }

                // PLACING SHIP
                    for(int i = 0; i < shipSize[ship]; i++) {
                        sb[rand_i][rand_j+i] = shipLetter[ship];
                    }
                    added = true;
                }

        // VERTICAL PLACEMENT ----------------------------
                else
                {
                // CHECKING FOR SPACE
                    bool spaceAvail = true;
                    for(int i = 0; i < shipSize[ship]; i++)
                    {
                        if(rand_i + i >= ROWS)
                        {
                            spaceAvail = false;
                            break;
                        }

                        if(     (*(*(sb+rand_i+i)+rand_j) != '~')    )
                        {
                            spaceAvail = false;
                            break;
                        }
                    }
                    if(!spaceAvail) {
                        // IF NO SPACE, RESTART LOOP
                        continue;
                    }

                // PLACING SHIP
                    for(int i = 0; i < CRUISER; i++) {
                        sb[rand_i+i][rand_j] = shipLetter[ship];
                    }
                    added = true;
                }
        }
}
}

void initialize(int *sock)
{
        // SHIP MEMORY INITIALIZATION
        printf("Initializing board...\n\n");

        sb = (char**)malloc(sizeof(char*)*ROWS);

        for (i=0; i < ROWS; i++)
        {
                *(sb + i) = (char*)malloc(sizeof(char)*COLUMNS);
        }


        for (i = 0; i < ROWS; i++)
        {
                for (j = 0; j < COLUMNS; j++)
                {
                        *(*(sb+i)+j) = '~';
                }
        }
        // END MEMORY ALLOCATION

        //PROOF OF SUCCESSFUL MEMORY ALLOCATION
        //DECOMMENT TO SEE ADDRESSES:
      /*
        for (i = 0; i < ROWS; i++)
        {
                printf("ADDRESS OF POINTER TO POINTER: %p ---> ", (sb+i));
                for (j = 0; j < COLUMNS; j++)
                {
                        printf(" %p ", *(sb+i)+j);
                        printf(" %c ", *(*(sb+i)+j));
                }
        printf("\n");
        }
       */

        // --------------------------
        // POPULATE SHIPS IN MEMORY
        addShips();


        // CREATE SERVER SOCKET

        int tempSocket;
        
		struct addrinfo hints, *info, *p;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		getaddrinfo(NULL, "7894",&hints, &info);

		for (p = info;p != NULL; p=p->ai_next) {
			if ((tempSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol))<0) continue;

                if (bind(tempSocket, p->ai_addr, p->ai_addrlen) < 0) { 

                perror("bind failed.");
                close(tempSocket);
                continue;
                }
			break;
		}

		if (p == NULL) 
			exit(1);

		char temp[1000];

		inet_ntop(AF_INET, &((struct sockaddr_in *)p->ai_addr)->sin_addr, temp, 1000);

		printf ("%s \n", temp);

		listen(tempSocket, 5);

		struct sockaddr_storage theirs;

		int size = sizeof(theirs);

		*sock = accept(tempSocket, (struct sockaddr *)&theirs, &size);

		freeaddrinfo(info);















}

//////////////////////////////
void acceptInput(char *rM, int *cM)
{
        char row;
        int col;

        //LETTER
        
        printf("\nEnter a character from A-J: ");
        scanf("%c%*c", &row);
        
        if (!(row >= 65 && row <=74))
        {
        do {
        printf("This is not a valid character, try again\n");
        printf("Enter a character from A-J: ");
        scanf("%c%*c", &row);
        }
        while (!(row >= 65 && row <=74));
        }

        //NUMBER

        printf("Enter a number from 0-9: ");
        scanf("%i%*c", &col);
        
        if (!(col >= 0 && col <=9))
        {
        do {
        printf("This is not a valid number, try again\n");
        printf("Enter a number from 0-9: ");
        scanf("%i%*c", &col);
        }
        while (!(col >= 0 && col <=9));
        }

        *rM = row;
        *cM = col;
        
}




/////////////////////////////////
void updateState(char row, int col, logfile **head_ref2)
{
        printf("\nEnemy Move: %c%d\n", row, col);
        int charMove;

        switch (row)
        {
                case 'A': charMove = 0;
                break;

                case 'B': charMove = 1;
                break;

                case 'C': charMove = 2;
                break;

                case 'D': charMove = 3;
                break;

                case 'E': charMove = 4;
                break;

                case 'F': charMove = 5;
                break;

                case 'G': charMove = 6;
                break;

                case 'H': charMove = 7;
                break;

                case 'I': charMove = 8;
                break;

                case 'J': charMove = 9;
                break;
        }

        bool shipHit = false;

        for (int i = 0; i < 5; i++)
        {
        if (sb[charMove][col] == shipLetter[i])
        {
                shipHit = true;
                printf("Hit! \nType: %s \n", shipNames[i]);
                board[charMove][col] = '!';
                sb[charMove][col] = 'x';
                addLog_hit(head_ref2, &row, col, shipNames[i], 1);
        }

        }

        if (!shipHit)
        {
                printf("Miss\n");
                board[charMove][col] = 'O';
                addLog_miss(head_ref2, &row, col, 0);
        }
        

        for (int ship = 0; ship < 5; ship++)
        {
        bool destroyed[5] = {true, true, true, true, true};

                for (int i = 0; i < ROWS; i++)
                {
                        for (j = 0; j < COLUMNS; j++)
                        {
                                if (sb[i][j] == shipLetter[ship])
                                {
                                       destroyed[ship] = false;
                                }
                        }
                }

                if ((destroyed[ship] == true) && (alreadyDown[ship] == false))
                {
                        printf("%s Destroyed\n", shipNames[ship]);
                        alreadyDown[ship] = true;
                        shipsDown++;
                        printf("\nShips Down : %d\n", shipsDown);
                }
                
                if (shipsDown == 5)
                {
                        gameOver = true;
                }
        }
}

////////////////////////////////////

void displayState() {

for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLUMNS; j++)
            {
                printf(" ");
                printf("%c", board[i][j]);
            }
        printf("\n");
    }

printf("\n");
//HIDDEN (COMPUTER) BOARD

for (i = 0; i < ROWS; i++)
        {
                for (j = 0; j < COLUMNS; j++)
                {
                        printf(" ");
                        printf("%c", sb[i][j]);
                }
        printf("\n");
        }

        
}

/////////////////////////////////////
void teardown(char **board, logfile **head_ref2, int socket) {
        

        printLog(*head_ref2);
        deleteLog(head_ref2);

        free(board);
        close(socket);
}

/////////////////////////////////////


int main() {

srand(time(0)); 

int sock;

char rowMove = 'x'; // CHAR = ROW
int colMove = 0; // INT = COLUMN

char enemy_rowMove;
int enemy_colMove;

logfile * head = NULL;

int turns = 0;

initialize(&sock);

generateBoard();


while (1)
{

memset(&rowMove, 0, sizeof(char));

acceptInput(&rowMove, &colMove);

int convert_col = htonl(colMove);

recv(sock, &enemy_rowMove, sizeof(char), 0);
recv(sock, &enemy_colMove, sizeof(uint32_t), 0);

send(sock, &rowMove, sizeof(char), 0);
send(sock, &convert_col, sizeof(uint32_t), 0);

int enemy_colMove_conv = ntohl(enemy_colMove);

updateState(enemy_rowMove, enemy_colMove_conv, &head);

displayState();

if (gameOver)
        {
                printf("Game Over! \nAll ships have been destroyed\n");
                break;
        }

}


teardown(sb, &head, sock);

}


