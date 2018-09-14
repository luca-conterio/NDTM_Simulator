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
#define RIGHT                    1
#define LEFT  			        -1
#define STOP  			         0
#define BLANK 			        '_'
#define DEFAULT_INPUT_DIM      256
#define INPUT_INCREMENT		   128
#define DEFAULT_PADDING_DIM     32
#define DEFAULT_STATES_DIM      32
#define STATES_INCREMENT        32
#define POSSIBLE_CHARS_NUM	   127
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

typedef struct tm_tape {
	char * string;
	int pointers_num;
} tm_tape;

typedef struct transition {
  int state;
  char in;
  char out;
  int move;
  int next_state;
  int index;
  tm_tape * tape;
  struct transition * next;
} transition;

void initGraph();
void readMTStructure();
void insertNodeInGraph(int, char, char, int, int);
void readInputStrings();
void run();
void executeTM();
tm_tape * modifyTapeChar(tm_tape *, int, char);
tm_tape * copyTape(tm_tape *);
void putInQueue(transition **, transition **, int, graph_node *, tm_tape *, int);
void removeFromQueue(transition **, transition **);
char * reallocTape(char *, int *);
void printGraph();
void printTape();
void printQueue();
void freeGraph();
void freeQueue();

int states_num = 0;						// the number of states of the TM
int states_dim = DEFAULT_STATES_DIM;    // the actual size of states array (graph)
state * graph;				    // array containing all the states read from input

transition * transitionsQueue = NULL;              // the queue of current possible transitions
transition * transitionsQueueTail = NULL;

tm_tape * tape;                            // the tape of the Turing Machine

int startingState = 0;                  // the starting state of the Turing Machine
long int currIteration;				// the current iteration
long int iterationsLimit;           // the limit to the iteration number (to avoid machine loop)

bool acceptString = false;              // true when a path accepts the input string
bool atLeastAnUndefinedPath = false;    // true when at least a path returns UNDEFINED (iteration > iterationsLimit)

int input_dim;							// the current length of the inputString array
char * inputString;						// the string read from input

int copiesNum = 0;
int reallocsNum = 0;

/***************************************************************
 * Initializes graph (states vector) elements to NULL
 ***************************************************************/
void initGraph() {
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
		graph[accState].isAccState = true;
		scanf("%s", in);
	}

	scanf("%ld", &iterationsLimit);    // read maximum number of iterations

	scanf("%s", in);                  // read the word "run" to start computations
	if (strcmp(in, "run") != 0)
		exit(0);
	else
		getchar(); // consume '\n' character after "run" string
}

/****************************************************************
 * Reads the next input string from stdin
 ****************************************************************/
void readInputStrings() {

	input_dim = DEFAULT_INPUT_DIM; 																			// actual size of the input string
	inputString = (char *) malloc(DEFAULT_INPUT_DIM); 		// current input string

	int i = 0;
	char c = ' ';

	while (c != EOF) {
		c = getchar();
		//printf("%d ", c);
		if (i == input_dim && c != EOF) {
			int newDim = input_dim + INPUT_INCREMENT;
			inputString = (char *) realloc(inputString, newDim);
			input_dim = newDim;
			if (DEBUG) printf("RIALLOCO VETTORE INPUT (dim = %d)\n", input_dim);
		}

		if (i != 0 && (c == '\n' || (c == EOF && inputString[i-1] != '\0'))) {
			inputString[i] = '\0';
			if (DEBUG) printf("input string: %s\n", inputString);
			if (inputString[i-1] != ' ' && inputString[i-1] != '\n')
				run();
			inputString = (char *) malloc(DEFAULT_INPUT_DIM);
			input_dim = DEFAULT_INPUT_DIM;
			i = 0;
		}
		else if (c != EOF && c != ' ') {
			inputString[i] = c;
			i++;
		}
	}

	free(inputString);
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
		graph[s].transitions[in] = new;
		new->next = NULL;
		return;
	}

	new->next = graph[s].transitions[in];
	graph[s].transitions[in] = new;

}

//***************************************
void run() {
	tape = (tm_tape *) malloc(sizeof(tm_tape));
	tape->string = (char *) malloc(strlen(inputString) + 1);
	tape->pointers_num = 0;
	strcpy(tape->string, inputString);
	free(inputString);
  acceptString = false;
  atLeastAnUndefinedPath = false;
	currIteration = 1;
  executeTM();
	if (acceptString == true)             // at least a path accpets the string ---> ACCEPT STRING
        printf("%c\n", ACCEPT);
	else if (atLeastAnUndefinedPath == true)   // at least an undefined value ---> UNDEFINED
		printf("%c\n", UNDEFINED);
	else printf("%c\n", REJECT);                        // only rejection values ---> REJECT STRING
    freeQueue();
}

//***************************************
void executeTM() {

	int accessibleTransitions = 0;
	graph_node * p = graph[0].transitions[tape->string[0]];

	// initialize for starting state (0) and starting index (0)
	while (p != NULL) {
		accessibleTransitions++;
		if (accessibleTransitions >= 2) {
			putInQueue(&transitionsQueue, &transitionsQueueTail, 0, p, copyTape(tape), 0);
			copiesNum++;
		}
		else
			putInQueue(&transitionsQueue, &transitionsQueueTail, 0, p, tape, 0);
		p = p->next;
	}

	/*if (accessibleTransitions == 0) // no accessible transitions from 0 ---> reject
		return;*/

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

			int next_state = currTransition->next_state;
			if (graph[next_state].isAccState == true)  { // accept string
				acceptString = true;
				return;
			}

			else {
				//char * currTape = currTransition->tape->string;

				if (currTransition->in == currTransition->tape->string[currTransition->index]) {

					currTransition->tape->string[currTransition->index] = currTransition->out;

					int next_index = currTransition->index + currTransition->move;
					if (next_index == -1 || next_index == strlen(currTransition->tape->string)-1) {
						currTransition->tape->string = reallocTape(currTransition->tape->string, &next_index);
					}

					if (graph[next_state].transitions != NULL) {
						char currChar = currTransition->tape->string[next_index];
						graph_node * p = graph[next_state].transitions[currChar];

						while (p != NULL) {
							accessibleTransitions++;
							if (accessibleTransitions >= 2) {
								putInQueue(&newQueue, &newQueueTail, next_state, p, copyTape(currTransition->tape), next_index);
								copiesNum++;
							}
							else
								putInQueue(&newQueue, &newQueueTail, next_state, p, currTransition->tape, next_index);
							p = p->next;
						}
					}
				}

				currTransition = currTransition->next;
				removeFromQueue(&transitionsQueue, &transitionsQueueTail);
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

/********************

*********************/
tm_tape * modifyTapeChar(tm_tape * currTape, int index, char out) {
	if (currTape->string[index] != out && currTape->pointers_num >= 1) {
		tm_tape * newTape = copyTape(currTape);
		newTape->string[index] = out;
		return newTape;
	}
	else
		return currTape;
}

/****************************************************************
* Returns a copy of the string passed as parameter
*****************************************************************/
tm_tape * copyTape(tm_tape * currTape) {
	tm_tape * newTape = (tm_tape *) malloc(sizeof(tm_tape));
	newTape->string = (char *) malloc(strlen(currTape->string)+1);
	strcpy(newTape->string, currTape->string);
	newTape->pointers_num = 0;

	if (DEBUG) {
		printf("\n\n###########   COPIO   ##########\n");
		printf("%p   %p\n", currTape, newTape);
	}

	return newTape;
}

/****************************************************************
* Puts a new element in the queue passed as parameter
*****************************************************************/
void putInQueue(transition ** queue, transition ** tail, int state, graph_node * p, tm_tape * tape, int index) {

	transition * new = (transition *) malloc(sizeof(transition));

	new->state = state;
	new->in = p->in;
	new->out = p->out;
	new->move = p->move;
	new->next_state = p->next_state;
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
void removeFromQueue(transition ** queue, transition ** tail) {

	if (queue == NULL)
		return;

	transition * toBeRemoved = *queue;
	*queue = (*queue)->next;

	if (toBeRemoved == *tail)
		*tail = NULL;

	toBeRemoved->next = NULL;

	if (DEBUG) printf("removed : %d %c %c %d %d %s %d\n", toBeRemoved->state, toBeRemoved->in, toBeRemoved->out, toBeRemoved->move, toBeRemoved->next_state, toBeRemoved->tape->string, toBeRemoved->index);

	toBeRemoved->tape->pointers_num--;

	if (toBeRemoved->tape->pointers_num == 0) {
		free(toBeRemoved->tape->string);
		free(toBeRemoved->tape);
	}

	free(toBeRemoved);
}

/****************************************************************
* Reallocates the current tape of the Turing Machine
*****************************************************************/
char * reallocTape(char * currTape, int * index) {

	int length = strlen(currTape)+1;

	if (*index == -1) {
		char * tapeCopy = (char *) malloc(length);
		strcpy(tapeCopy, currTape);
		currTape = realloc(currTape, length + DEFAULT_PADDING_DIM);
		for (int i = 0; i < DEFAULT_PADDING_DIM; i++)
			currTape[i] = BLANK;
		strcpy(&currTape[DEFAULT_PADDING_DIM], tapeCopy);
		free(tapeCopy);

		*index = *index + DEFAULT_PADDING_DIM;
  }
  else {
		currTape = realloc(currTape, length + DEFAULT_PADDING_DIM);
		for (int i = length-1; i < length + DEFAULT_PADDING_DIM; i++)
			currTape[i] = BLANK;
  }

	currTape[length+DEFAULT_PADDING_DIM-1] = '\0';
	if (DEBUG) printf("\nRIALLOCO VETTORE TAPE (dim = %d)\n\n", (int) strlen(currTape));
	reallocsNum++;
	return currTape;
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

/*****************************************************************
 * Frees the transitions queue
 *****************************************************************/
 void freeQueue() {
	 transition * p = transitionsQueue;

	 while(transitionsQueue != NULL) {
		 transitionsQueue = transitionsQueue->next;
		 free(p->tape->string);
		 free(p->tape);
		 free(p);
		 p = transitionsQueue;
	 }

	 transitionsQueue = NULL;
	 transitionsQueueTail = NULL;
 }

/****************************************************************
 * Displays the graph_nodes graph
 ****************************************************************/
void printGraph() {
	graph_node * p;
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
					printf("\n%d %c %c %c %d", i, p->in, p->out, m, p->next_state);
					p = p->next;
				}
			}
		}
	}
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
			switch(p->move) {
				case 0: m = 'S'; break;
				case 1: m = 'R'; break;
				case -1: m = 'L'; break;
			}
			printf("%d %c %c %d %d %s %d\n", p->state, p->in, p->out, m, p->next_state, p->tape->string, p->index);
			p = p->next;
		}
		printf("\n");
	}
}

/****************************************************************
 * Displays the tape in the current state
 ****************************************************************/
void printTape(int i, char * t) {
	for (int j = 0; j < strlen(t); j++)
		printf("%c", t[j]);
	printf("\n");
	for (int j = 0; j < i; j++)
		printf("%c", ' ');
	printf("∆\n");
}

/**************************************************************
 * 						 Main function
 **************************************************************/
int main(int argc, char * argv[]) {
	graph = (state *) malloc(DEFAULT_STATES_DIM * sizeof(state));
	initGraph();
	readMTStructure();

  if (DEBUG) {
  	printf("\nStates number: %d", states_num);
  	printf("\nIterations limit: %ld", iterationsLimit);
  	printGraph();
  	printf("\n");
  }

	readInputStrings();

    //freeGraph();
	//printf("number of copies: %d\n", copiesNum);
	//printf("number of reallocs: %d\n", reallocsNum);

	return 0;
}
