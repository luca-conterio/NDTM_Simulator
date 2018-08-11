//
//  main.c
//  ndtm
//
//  Created by Luca Conterio on 25/05/18.
//  Copyright © 2018 Luca Conterio. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ACCEPT                  '1'
#define REJECT                  '0'
#define UNDEFINED	            'U'
#define BLANK 			        '_'
#define RIGHT                    1
#define LEFT  			        -1
#define STOP  			         0
#define DEFAULT_INPUT_DIM      256
#define INPUT_INCREMENT		   128
#define DEFAULT_STATES_DIM      64
#define STATES_INCREMENT        64
#define POSSIBLE_CHARS_NUM	    58
#define TAPE_CHUNK_LENGTH	 511+1
#define DEBUG 					 0

typedef enum {true, false} bool;

typedef struct graph_node {       // node of the graph
	char in;
	char out;
	int move;
	int next_state;
	struct graph_node * next;     // list of all possible next states of the current one
} graph_node;

typedef struct state {
	graph_node ** transitions;
	bool isAccState;
} state;

typedef struct tape_chunk {
    char * string;
    struct tape_chunk * right;
	struct tape_chunk * left;
} tape_chunk;

typedef struct tm_tape {
	tape_chunk * head;
    tape_chunk * currChunk;
	int pointers_num;
} tm_tape;

typedef struct transition {
  int state;
  graph_node * transition;
  int index;
  tm_tape * tape;
  struct transition * next;
} transition;

void init();
void readMTStructure();
void insertNodeInGraph(int, char, char, int, int);
void readInputStrings();
tape_chunk * addTapeChunk(tm_tape *, char *);
void initTapeChunk(tape_chunk *);
void run();
void executeTM();
tm_tape * copyTape(tm_tape *);
void putInTransitionsQueue(transition **, transition **, int, graph_node *, tm_tape *, int);
void removeFromTransitionsQueue(transition **, transition **);
tm_tape * updateIndex(tm_tape *, int *);
void prependNewTapeChunk(tm_tape **);
void appendNewTapeChunk(tape_chunk **);
tape_chunk * createNewChunk();
tm_tape * copyTape(tm_tape *);
void printGraph();
void printTape(tm_tape *);
void printQueue();
void freeGraph();
void freeQueue();
void freeTape(tape_chunk *);

int states_num = 0;						     // the number of states of the TM
int states_dim = DEFAULT_STATES_DIM;         // the actual size of states array (graph)
state * graph;				                 // array containing all the states read from input

transition * transitionsQueue = NULL;        // the queue of current possible transitions
transition * transitionsQueueTail = NULL;

tm_tape * tape;                             // the tape of the Turing Machine

int startingState = 0;                      // the starting state of the Turing Machine
long int currIteration;				        // the current iteration
long int iterationsLimit;                   // the limit to the iteration number (to avoid machine loop)

bool acceptString = false;                  // true when a path accepts the input string
bool atLeastAnUndefinedPath = false;        // true when at least a path returns UNDEFINED (iteration > iterationsLimit)

int input_dim;							    // the current length of the inputString array
char * inputString;						    // the string read from input

int copiesNum = 0;
int reallocsNum = 0;
int freesNum = 0;

/***************************************************************
 * Initializes graph (states vector) elements to NULL
 ***************************************************************/
void init() {
    tape = (tm_tape *) malloc(sizeof(tm_tape));
    tape->head = NULL;
    tape->currChunk = NULL;
	for(int i = 0; i < DEFAULT_STATES_DIM; i++) {
		graph[i].transitions = NULL;
		graph[i].isAccState = false;
	}
}

/*******************************************************************
 * Reads the stdin (MT structure) saving data in program structures
 *******************************************************************/
void readMTStructure() {
	int s = -1;
	char inputChar = ' ';
	char outputChar = ' ';
	char move = ' ';
	int next_s = -1;
	char in[5];

	scanf("%s", in);
	if (strcmp(in, "tr") != 0)
		exit(0);

	scanf("%s", in);
	while (strcmp(in, "acc") != 0) { 	// cycle until find the word "acc"
		sscanf(in, "%d", &s); 		 	// read state
		scanf("%s", in);
		inputChar = in[0];    		    // read input character
		scanf("%s", in);
		outputChar = in[0];   		 	// read output character
		scanf("%s", in);				// read move
		switch(in[0]) {
			case 'R': move = RIGHT;
					  break;
			case 'L': move = LEFT;
					  break;
			case 'S': move = STOP;
					  break;
			default:  printf("Invalid move character\n");
					  break;
		}
		scanf("%s", in);
		sscanf(in, "%d", &next_s);   					// read next state
		if (s >= states_num || next_s >= states_num) {	// maintain the maximum states number
			if (s >= next_s)
				states_num = s + 1;
			else
				states_num = next_s + 1;
		}
		if (states_num >= states_dim) {
			int newDim = states_num + STATES_INCREMENT;
			graph = (state *) realloc(graph, newDim * sizeof(state));
			// re-initialize states vector (graph) with NULL in new positions
			for (int i = states_dim; i < newDim; i++) {
				graph[i].transitions = NULL;
				graph[i].isAccState = false;
			}
			states_dim = newDim;
			if (DEBUG) printf("RIALLOCO VETTORE STATI (dim = %d)\n", states_dim);
		}
		insertNodeInGraph(s, inputChar, outputChar, move, next_s);
		scanf("%s", in);
	}

	int accState;
	scanf("%s", in);
	while (strcmp(in, "max") != 0) { 	// read acceptation states
		sscanf(in, "%d", &accState);
        if (DEBUG) printf("\nAcc state: %d", accState);
		graph[accState].isAccState = true;
		scanf("%s", in);
	}

	scanf("%ld", &iterationsLimit);    // read maximum number of iterations

	scanf("%s", in);                   // read the word "run" to start computations
	if (strcmp(in, "run") != 0)
		exit(0);
	else
		getchar(); // consume '\n' character after "run" string
}

/****************************************************************
 * Reads the next input string from stdin
 ****************************************************************/
void readInputStrings() {
    char inputString[TAPE_CHUNK_LENGTH]; // current input string

    int i = 0;
    char c = ' ';

    while (c != EOF) {
        c = getchar();
        if (i == TAPE_CHUNK_LENGTH-1 && c != EOF && c != '\n') {
            inputString[i] = '\0';
            initTapeChunk(addTapeChunk(tape, inputString));
            i = 0;
        }

        if (i != 0 && (c == '\n' || (c == EOF && inputString[i-1] != '\0'))) {
            inputString[i] = '\0';
            if (strlen(inputString) != 0)
                initTapeChunk(addTapeChunk(tape, inputString));
            run();
            tape = (tm_tape *) malloc(sizeof(tm_tape));
            tape->head = NULL;
            tape->currChunk = NULL;
            i = 0;
        }
        else if (c != EOF && c != '\n') {
            inputString[i] = c;
            i++;
        }
    }
}

//*****************************************************************
tape_chunk * addTapeChunk(tm_tape * t, char * string) {
	tape_chunk * new = (tape_chunk *) malloc(sizeof(tape_chunk));
	new->string = (char *) malloc(TAPE_CHUNK_LENGTH);

	if (new == NULL) {
		printf("Error: not enough memory...");
		exit(0);
	}

	if (t->head == NULL)
		t->head = new;

	strcpy(new->string, string);
	new->left = t->currChunk;

	if (t->currChunk != NULL)
		t->currChunk->right = new;

	new->right = NULL;
	t->currChunk = new;

    return t->currChunk;
}

/******************************************************************
* Initializes a new tape chunk with BLANK characters
*******************************************************************/
void initTapeChunk(tape_chunk * chunk) {
    if (strlen(chunk->string) < TAPE_CHUNK_LENGTH) { // add BLANK characters to fill the chunk char array
		for (int i = strlen(chunk->string); i < TAPE_CHUNK_LENGTH; i++) {
			chunk->string[i] = BLANK;
		}
		chunk->string[TAPE_CHUNK_LENGTH-1] = '\0';
	}
}

/******************************************************************
 * Inserts the graph_node read from stdin in the graph_nodes graph
 ******************************************************************/
void insertNodeInGraph(int s, char in, char out, int m, int n_s) {

	graph_node * new = (graph_node *) malloc(sizeof(graph_node));
	if (new != NULL) {
		new->in = in;
		new->out = out;
		new->move = m;
		new->next_state = n_s;
	}
	else {
		printf("Error: not enough memory...");
		exit(0);
	}

	if (graph[s].transitions == NULL) {   // insert when list is empty
		graph[s].transitions = (graph_node **) malloc(POSSIBLE_CHARS_NUM * sizeof(graph_node *));
		for (int i = 0; i < POSSIBLE_CHARS_NUM; i++)
			graph[s].transitions[i] = NULL;
		graph[s].transitions[in-65] = new;
		new->next = NULL;
		return;
	}

	new->next = graph[s].transitions[in-65];
	graph[s].transitions[in-65] = new;

}

//***************************************
void run() {
    tape->currChunk = tape->head;
    tape->pointers_num = 0;

    if (DEBUG) printTape(tape);

    acceptString = false;
    atLeastAnUndefinedPath = false;
	currIteration = 1;

    executeTM();
	freeQueue();

	if (acceptString == true)             // at least a path accpets the string ---> ACCEPT STRING
        printf("%c\n", ACCEPT);
	else if (atLeastAnUndefinedPath == true)   // at least an undefined value ---> UNDEFINED
		printf("%c\n", UNDEFINED);
	else printf("%c\n", REJECT);                        // only rejection values ---> REJECT STRING
}

/*****************************************************************
 * Actually executes the Turing Machine on the given input
 *****************************************************************/
void executeTM() {
    int accessibleTransitions = 0;
	graph_node * p = graph[0].transitions[tape->currChunk->string[0]-65];

	// initialize for starting state (0) and starting index (0)
	while (p != NULL) {
		accessibleTransitions++;
		/*if (p->in == p->out && p->next_state == 0 && p->move == STOP)
			atLeastAnUndefinedPath = true;
		else*/ if (accessibleTransitions >= 2)
			putInTransitionsQueue(&transitionsQueue, &transitionsQueueTail, 0, p, copyTape(tape), 0);
		else
			putInTransitionsQueue(&transitionsQueue, &transitionsQueueTail, 0, p, tape, 0);
		p = p->next;
	}

    // start algorithm
	transition * currTransition = transitionsQueue;
	transition * newQueue = NULL;
	transition * newQueueTail = NULL;
	accessibleTransitions = 0;

	while (transitionsQueue != NULL && currIteration <= iterationsLimit) {

		if (DEBUG) {
			printQueue();
			printf("iteration: %ld\n", currIteration);
		}

		while (currTransition != NULL) {

			int next_state = currTransition->transition->next_state;
			if (graph[next_state].isAccState == true)  { // accept string
				acceptString = true;
				return;
			}

			else {
				//char * currTape = currTransition->tape->string;

				if (currTransition->transition->in == currTransition->tape->currChunk->string[currTransition->index]) {

					currTransition->tape->currChunk->string[currTransition->index] = currTransition->transition->out;

					int next_index = currTransition->index + currTransition->transition->move;
                    if (next_index == -1 || next_index == TAPE_CHUNK_LENGTH-1)
                        currTransition->tape = updateIndex(currTransition->tape, &next_index);

					if (graph[next_state].transitions != NULL) {
						char currChar = currTransition->tape->currChunk->string[next_index];
						graph_node * p = graph[next_state].transitions[currChar-65];

						while (p != NULL) {
							accessibleTransitions++;
							/*if (p->in == p->out && currTransition->state == p->next_state && p->move == STOP)
								atLeastAnUndefinedPath = true;
							else*/ if (accessibleTransitions >= 2)
								putInTransitionsQueue(&newQueue, &newQueueTail, next_state, p, copyTape(currTransition->tape), next_index);
							else
								putInTransitionsQueue(&newQueue, &newQueueTail, next_state, p, currTransition->tape, next_index);
							p = p->next;
						}
					}
				}

				currTransition = currTransition->next;
				removeFromTransitionsQueue(&transitionsQueue, &transitionsQueueTail);
				accessibleTransitions = 0;
			}
		}

		transitionsQueue = newQueue;
		transitionsQueueTail = newQueueTail;
		currTransition = transitionsQueue;
		newQueue = NULL;
		newQueueTail = NULL;
		currIteration++;
	}

	if (currIteration > iterationsLimit)   // undefined value for computation
		atLeastAnUndefinedPath = true;
}

/****************************************************************
* Puts a new element in the queue passed as parameter
*****************************************************************/
void putInTransitionsQueue(transition ** queue, transition ** tail, int state, graph_node * p, tm_tape * tape, int index) {

	transition * new = (transition *) malloc(sizeof(transition));

	new->state = state;
	new->transition = p;
	new->index = index;
	new->tape = tape;
	new->tape->pointers_num++;

	if (*queue == NULL) {
		*queue = new;
	}

	if (*tail != NULL)
		(*tail)->next = new;

	new->next = NULL;
	*tail = new;
}

/****************************************************************
* Removes the first element of the queue passed as parameter
*****************************************************************/
void removeFromTransitionsQueue(transition ** queue, transition ** tail) {

	if (queue == NULL)
		return;

	transition * toBeRemoved = *queue;
	*queue = (*queue)->next;

	if (toBeRemoved == *tail)
		*tail = NULL;

	toBeRemoved->next = NULL;

	toBeRemoved->tape->pointers_num--;

	if (toBeRemoved->tape->pointers_num == 0) {
		freeTape(toBeRemoved->tape->head);
		free(toBeRemoved->tape);
		if (DEBUG) printf("!!!!!!!!!!!!! >>>>>>>   freed tape string   <<<<<<< !!!!!!!!!!!!!\n");
		freesNum++;
	}

	free(toBeRemoved);
}

//****************************************************************
tm_tape * updateIndex(tm_tape * t, int * index) {

    if (*index == -1) {
        if (t->currChunk->left == NULL)
            prependNewTapeChunk(&t);
        t->currChunk = t->currChunk->left;
        *index = strlen(t->currChunk->string)-1;         // exclude the '\0'
    }
    else if (*index == TAPE_CHUNK_LENGTH-1) {
        if (t->currChunk->right == NULL)
            appendNewTapeChunk(&t->currChunk);
        t->currChunk = t->currChunk->right;
        *index = 0;
    }

    return t;
}

/*****************************************************************
 * Prepends to a given tape_chunk a new one
 *****************************************************************/
void prependNewTapeChunk(tm_tape ** t) {
    tape_chunk * newChunk = createNewChunk();
    newChunk->right = (*t)->currChunk;
    ((*t)->currChunk)->left = newChunk;
	(*t)->head = newChunk;

	reallocsNum++;
    if (DEBUG) printf("#######   PREPEND NEW CHUNK   #######\n");
}

/*****************************************************************
 * Appends to a given tape_chunk a new one
 *****************************************************************/
void appendNewTapeChunk(tape_chunk ** current) {
    tape_chunk * newChunk = createNewChunk();
    (*current)->right = newChunk;
    newChunk->left = *current;

	reallocsNum++;
    if (DEBUG) printf("#######   APPEND NEW CHUNK   #######\n");
}

/*****************************************************************
 * Creates a new BLANK tape_chunk
 *****************************************************************/
tape_chunk * createNewChunk() {
    tape_chunk * newChunk = (tape_chunk *) malloc(sizeof(tape_chunk));
    newChunk->string = (char *) malloc(TAPE_CHUNK_LENGTH);
    newChunk->string[0] = '\0';
    initTapeChunk(newChunk);
    newChunk->string[TAPE_CHUNK_LENGTH-1] = '\0';
    newChunk->left = NULL;
    newChunk->right = NULL;
    return newChunk;
}

/*****************************************************************
 * Returns a copy of the given tape
 *****************************************************************/
tm_tape * copyTape(tm_tape * currTape) {

    tape_chunk * p = currTape->head;
    tape_chunk * current = NULL;
    tm_tape * newTape = (tm_tape *) malloc(sizeof(tm_tape));
    newTape->head = NULL;
    newTape->currChunk = NULL;
	newTape->pointers_num = 0;

    while (p != NULL) {
        tape_chunk * newChunk = addTapeChunk(newTape, p->string);
        if (p == currTape->currChunk) {
            current = newChunk;
		}
        p = p->right;
    }

    newTape->currChunk = current;

	if (DEBUG) printf("#######   COPY TAPE   #######\n");
    copiesNum++;

    return newTape;
}

/*****************************************************************
 * Frees the transitions queue
 *****************************************************************/
 void freeQueue() {
	 transition * p = transitionsQueue;

	 while(transitionsQueue != NULL) {
		 transitionsQueue = transitionsQueue->next;
		 freeTape(p->tape->head);
		 free(p->tape);
		 free(p);
		 p = transitionsQueue;
		 //removeFromTransitionsQueue(&transitionsQueue, &transitionsQueueTail);
		 freesNum++;
	 }

	 transitionsQueue = NULL;
	 transitionsQueueTail = NULL;
 }

 /*****************************************************************
  * Frees the given tape
  *****************************************************************/
 void freeTape(tape_chunk * head) {
 	tape_chunk * prec = head;
 	tape_chunk * succ = head;
 	while (succ != NULL) {
 		succ = succ->right;
 		free(prec->string);
 		free(prec);
 		prec = succ;
 	}
 }

 /*****************************************************************
  * Frees the Turing Machine graph
  *****************************************************************/
 void freeGraph() {
 	int i = 0;
 	graph_node * prec = NULL;
 	graph_node * succ = NULL;
 	while (i < states_dim) {
 		if (graph[i].transitions != NULL) {
 			for (int j = 0; j < POSSIBLE_CHARS_NUM; j++) {
 				prec = graph[i].transitions[j];
 				succ = graph[i].transitions[j];
 				while (succ != NULL) {
 					succ = succ->next;
 					free(prec);
 					prec = succ;
 				}
 			}
 			free(graph[i].transitions);
 		}
 		i++;
 	}
 	free(graph);
 }

/****************************************************************
 * Displays the graph_nodes graph
 ****************************************************************/
void printGraph() {
	graph_node * p;
    printf("\nGRAPH: \n");
	for (int i = 0; i < states_num; i++) {
		if (graph[i].transitions != NULL) {
			for (int j = 0; j < POSSIBLE_CHARS_NUM; j++) {
				p = graph[i].transitions[j];
				while (p != NULL) {
					char m;
					switch(p->move) {
						case 0: m = 'S'; break;
						case 1: m = 'R'; break;
						case -1: m = 'L'; break;
					}
					printf("%d %c %c %c %d\n", i, p->in, p->out, m, p->next_state);
					p = p->next;
				}
			}
		}
	}
}

//******************************************************************
void printTape(tm_tape * currTape) {
	tape_chunk * p = currTape->head;
	int i = 0;
	while (p != NULL) {
		printf("%s", p->string);
		p = p->right;
		i++;
	}
    printf("\n");
}

/****************************************************************
 * Displays the current possible transitions queue
 ****************************************************************/
void printQueue() {
	transition * p = transitionsQueue;
	printf("\n");
	if (transitionsQueue == NULL)
	   printf("queue: NULL\n");
	else {
		printf("queue: \n");
		while(p != NULL) {
			char m;
			switch(p->transition->move) {
				case 0: m = 'S'; break;
				case 1: m = 'R'; break;
				case -1: m = 'L'; break;
			}
			printf("%d %c %c %d %d   index: %d\n", p->state, p->transition->in, p->transition->out, m, p->transition->next_state, p->index);
            printTape(p->tape);
			p = p->next;
		}
		printf("\n");
	}
}

/**************************************************************
 * 						 Main function
 **************************************************************/
int main(int argc, char * argv[]) {
	graph = (state *) malloc(DEFAULT_STATES_DIM * sizeof(state));
	init();
	readMTStructure();

	if (DEBUG) {
		printf("\nStates number: %d (from 0 to %d)", states_num, states_num-1);
		printf("\nIterations limit: %ld\n", iterationsLimit);
		printGraph();
		printf("\n");
	}

	readInputStrings();

    //freeGraph();
	//printf("number of copies: %d\n", copiesNum);
	//printf("number of reallocs: %d\n", reallocsNum);
	//printf("number of frees: %d\n", freesNum);


	return 0;
}
