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
#define DEFAULT_STATES_DIM     256
#define STATES_INCREMENT       256
#define POSSIBLE_CHARS_NUM	   127
#define MIN_CHAR				 0
#define QUEUE_DIM			   512
#define TAPE_CHUNK_LENGTH	   512

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
} transition;

void init();
void readMTStructure();
void insertNodeInGraph(int, char, char, int, int);
void readInputStrings();
tape_chunk * addTapeChunk(tm_tape *, char *);
void initTapeChunk(tape_chunk *, int);
void run();
void executeTM();
tm_tape * copyTape(tm_tape *);
void putInTransitionsQueue(int, graph_node *, tm_tape *, int);
void removeFromTransitionsQueue();
tm_tape * updateIndex(tm_tape *, int *);
void prependNewTapeChunk(tm_tape **);
void appendNewTapeChunk(tape_chunk **);
tape_chunk * createNewChunk();
void printGraph();
void printTape(tm_tape *);
void printQueue();
void freeGraph();
void freeQueue();
void freeTape(tape_chunk *);

int states_num = 0;						    // the number of states of the TM
int states_dim = DEFAULT_STATES_DIM;        // the actual size of states array (graph)
state * graph;				                // array containing all the states read from input

transition transitionsQueue[QUEUE_DIM];     // the queue of current possible transitions
int queueHead;
int queueTail;

tm_tape * tape;                             // the tape of the Turing Machine

int startingState = 0;                      // the starting state of the Turing Machine
long int currIteration;				        // the current iteration
long int iterationsLimit;                   // the limit to the iteration number (to avoid machine loop)

bool acceptString = false;                  // true when a path accepts the input string
bool atLeastAnUndefinedPath = false;        // true when at least a path returns UNDEFINED (iteration > iterationsLimit)

int input_dim;							    // the current length of the inputString array
char * inputString;						    // the string read from input

int nodesNum = 0;                           // the number of nodes in the computation queue

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
			default:  break;
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
		}
		insertNodeInGraph(s, inputChar, outputChar, move, next_s);
		scanf("%s", in);
	}

	int accState;
	scanf("%s", in);
	while (strcmp(in, "max") != 0) { 	// read acceptation states
		sscanf(in, "%d", &accState);
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
            addTapeChunk(tape, inputString);
            i = 0;
        }

        if (i != 0 && (c == '\n' || (c == EOF && inputString[i-1] != '\0'))) {
            inputString[i] = '\0';
            initTapeChunk(addTapeChunk(tape, inputString), i);
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

	if (t->head == NULL)
		t->head = new;

	memcpy(new->string, string, TAPE_CHUNK_LENGTH);
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
void initTapeChunk(tape_chunk * chunk, int length) {
	memset(&chunk->string[length], BLANK, TAPE_CHUNK_LENGTH-length);
	chunk->string[TAPE_CHUNK_LENGTH-1] = '\0';
}

/******************************************************************
 * Inserts the graph_node read from stdin in the graph_nodes graph
 ******************************************************************/
void insertNodeInGraph(int s, char in, char out, int m, int n_s) {

	graph_node * new = (graph_node *) malloc(sizeof(graph_node));

	new->in = in;
	new->out = out;
	new->move = m;
	new->next_state = n_s;

	if (graph[s].transitions == NULL) {   // insert when list is empty
		graph[s].transitions = (graph_node **) malloc(POSSIBLE_CHARS_NUM * sizeof(graph_node *));
		for (int i = 0; i < POSSIBLE_CHARS_NUM; i++)
			graph[s].transitions[i] = NULL;
		graph[s].transitions[in-MIN_CHAR] = new;
		new->next = NULL;
		return;
	}

	new->next = graph[s].transitions[in-MIN_CHAR];
	graph[s].transitions[in-MIN_CHAR] = new;

}

//***************************************
void run() {
    tape->currChunk = tape->head;
    tape->pointers_num = 0;

    acceptString = false;
    atLeastAnUndefinedPath = false;
	currIteration = 1;
	nodesNum = 0;
	queueHead = 0;
	queueTail = 0;

    executeTM();
	freeQueue();

	if (acceptString == true)             // at least a path accpets the string ---> ACCEPT STRING
        printf("%c\n", ACCEPT);
	else if (atLeastAnUndefinedPath == true)   // at least an undefined value ---> UNDEFINED
		printf("%c\n", UNDEFINED);
	else printf("%c\n", REJECT);
}

/*****************************************************************
 * Actually executes the Turing Machine on the given input
 *****************************************************************/
void executeTM() {
    int accessibleTransitions = 0;
	graph_node * p = graph[0].transitions[tape->currChunk->string[0]-MIN_CHAR];

	// initialize for starting state (0) and starting index (0)
	while (p != NULL) {
		accessibleTransitions++;
		if (accessibleTransitions >= 2)
			putInTransitionsQueue(0, p, copyTape(tape), 0);
		else
			putInTransitionsQueue(0, p, tape, 0);
		p = p->next;
	}

	if (nodesNum == 0) return; // no possible transitions from state 0

    // start algorithm
	int currentTail = queueTail;
	transition currTransition = transitionsQueue[queueHead];
	accessibleTransitions = 0;

	while (nodesNum != 0 && currIteration <= iterationsLimit) {

		while (queueHead != currentTail) {

			int next_state = currTransition.transition->next_state;

			if (graph[next_state].isAccState == true)  { // accept string
				acceptString = true;
				return;
			}

			else if (currTransition.state == currTransition.transition->next_state && (currTransition.transition->in == BLANK || (currTransition.transition->in == currTransition.transition->out && currTransition.transition->move == STOP))) {
				atLeastAnUndefinedPath = true;
			}

			else {

				if (currTransition.transition->in == currTransition.tape->currChunk->string[currTransition.index]) {

					currTransition.tape->currChunk->string[currTransition.index] = currTransition.transition->out;

					int next_index = currTransition.index + currTransition.transition->move;
                    if (next_index == -1 || next_index == TAPE_CHUNK_LENGTH-1)
                        currTransition.tape = updateIndex(currTransition.tape, &next_index);

					if (graph[next_state].transitions != NULL) {
						char currChar = currTransition.tape->currChunk->string[next_index];
						graph_node * p = graph[next_state].transitions[currChar-MIN_CHAR];

						while (p != NULL) {
							accessibleTransitions++;
							if (accessibleTransitions >= 2)
								putInTransitionsQueue(next_state, p, copyTape(currTransition.tape), next_index);
							else
								putInTransitionsQueue(next_state, p, currTransition.tape, next_index);
							p = p->next;
						}
					}
				}
			}

			removeFromTransitionsQueue();
			currTransition = transitionsQueue[queueHead];
			accessibleTransitions = 0;
		}

		currentTail = queueTail;
		currTransition = transitionsQueue[queueHead];
		currIteration++;
	}

	if (currIteration > iterationsLimit)   // undefined value for computation
		atLeastAnUndefinedPath = true;
}

/****************************************************************
* Puts a new element in the queue passed as parameter
*****************************************************************/
void putInTransitionsQueue(int state, graph_node * p, tm_tape * tape, int index) {

	transitionsQueue[queueTail].state = state;
	transitionsQueue[queueTail].transition = p;
	transitionsQueue[queueTail].index = index;
	transitionsQueue[queueTail].tape = tape;
	transitionsQueue[queueTail].tape->pointers_num++;

	if (queueTail == QUEUE_DIM-1)
		queueTail = 0;
	else
		queueTail++;

	nodesNum++;
}

/****************************************************************
* Removes the first element of the queue passed as parameter
*****************************************************************/
void removeFromTransitionsQueue() {

	if (nodesNum == 0)
		return;

	transitionsQueue[queueHead].tape->pointers_num--;

	if (transitionsQueue[queueHead].tape->pointers_num == 0) {
		freeTape(transitionsQueue[queueHead].tape->head);
		free(transitionsQueue[queueHead].tape);
	}

	if (queueHead == QUEUE_DIM-1)
		queueHead = 0;
	else
		queueHead++;

	nodesNum--;
}

//****************************************************************
tm_tape * updateIndex(tm_tape * t, int * index) {

    if (*index == -1) {
        if (t->currChunk->left == NULL)
            prependNewTapeChunk(&t);
        t->currChunk = t->currChunk->left;
        *index = TAPE_CHUNK_LENGTH-2;         // exclude the '\0'
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
}

/*****************************************************************
 * Appends to a given tape_chunk a new one
 *****************************************************************/
void appendNewTapeChunk(tape_chunk ** current) {
    tape_chunk * newChunk = createNewChunk();
    (*current)->right = newChunk;
    newChunk->left = *current;
}

/*****************************************************************
 * Creates a new BLANK tape_chunk
 *****************************************************************/
tape_chunk * createNewChunk() {
    tape_chunk * newChunk = (tape_chunk *) malloc(sizeof(tape_chunk));
    newChunk->string = (char *) malloc(TAPE_CHUNK_LENGTH);
    initTapeChunk(newChunk, 0);
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

    return newTape;
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

//***************************************************************
void freeQueue() {
	 int i = queueHead;
	 while(i != queueTail) {
	 	freeTape(transitionsQueue[i].tape->head);
		free(transitionsQueue[i].tape);
		if (i == QUEUE_DIM-1)
			i = 0;
		else
			i++;
	}
 }

/**************************************************************
 * 						 Main function
 **************************************************************/
int main(int argc, char * argv[]) {
	graph = (state *) malloc(DEFAULT_STATES_DIM * sizeof(state));
	init();
	readMTStructure();
	readInputStrings();
	return 0;
}
