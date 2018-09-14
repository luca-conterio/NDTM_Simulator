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

#define ACCEPT                 '1'
#define REJECT                 '0'
#define UNDEFINED	           'U'
#define RIGHT                   1
#define LEFT  			       -1
#define STOP  			        0
#define BLANK 			       '_'
#define DEFAULT_INPUT_DIM       256
#define INPUT_INCREMENT		    128
#define DEFAULT_PADDING_DIM     16
#define DEFAULT_STATES_DIM      16
#define STATES_INCREMENT        16
#define DEBUG 					0

typedef enum {true, false} bool;

typedef struct transition {       // node of the graph
	char in;
	char out;
	int move;
	int next_state;
	struct transition * next;     // list of all possible next states of the current one
} transition;

typedef struct state {
	transition * transitionsList;
	bool isAccState;
} state;

typedef struct stack_node {
  char * tape;
  struct stack_node * next;
} stack_node;

typedef struct acc_state {
	int value;
	struct acc_state * next;
} acc_state;

void initGraph();
void initTape();
void readMTStructure();
void addAcceptationState(int);
void insertTransitionInGraph(int, char, char, int, int);
void readInputStrings();
void run();
void executeTM(int, int, unsigned int);
void performTransition(char *, int *, int *, unsigned int *);
void performNonDeterministicTransition(char *, int, int, unsigned int);
void putInStack(char *);
void popFromStack();
int countAccessibleTransitions(int, char);
char * reallocTape(char *, int *);
bool checkIfAccState(int);
char checkComputationResult();
void printGraph();
void printTape();
void printStack();
void freeGraph();
void freeStack();
void freeAccStatesList();

int states_num = 0;						// the number of states of the TM
int states_dim = DEFAULT_STATES_DIM;    // the actual size of states array (graph)
state * graph;				    // array containing all the states read from input

stack_node * stack = NULL;              // the stack used to handle non-deterministic moves

char * tape;                            // the tape of the Turing Machine

int startingState = 0;                  // the starting state of the Turing Machine
int reject_state = -1;    				// the state of rejection
unsigned int iterationsLimit;           // the limit to the iteration number (to avoid machine loop)

bool acceptString = false;              // true when a path accepts the input string
bool atLeastAnUndefinedPath = false;    // true when at least a path returns UNDEFINED (iteration > iterationsLimit)

int input_dim;							// the current length of the inputString array
char * inputString;						// the string read from input

/***************************************************************
 * Initializes graph (states vector) elements to NULL
 ***************************************************************/
void initGraph() {
	for(int i = 0; i < DEFAULT_STATES_DIM; i++) {
		graph[i].transitionsList = NULL;
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
		if (s >= states_num || next_s >= states_num) {	// maintain the maximum state number
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
				graph[i].transitionsList = NULL;
				graph[i].isAccState = false;
			}
			states_dim = newDim;
			if (DEBUG) printf("RIALLOCO VETTORE STATI (dim = %d)\n", states_dim);
		}
		insertTransitionInGraph(s, inputChar, outputChar, move, next_s);
		scanf("%s", in);
	}

	int accState;
	scanf("%s", in);
	while (strcmp(in, "max") != 0) { 	// read acceptation states
		sscanf(in, "%d", &accState);
		graph[accState].isAccState = true;
		scanf("%s", in);
	}

	scanf("%u", &iterationsLimit);    // read maximum number of iterations

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
 * Inserts the transition read from stdin in the transitions graph
 ******************************************************************/
void insertTransitionInGraph(int s, char in, char out, int m, int n_s) {

	transition * new = (transition *) malloc(sizeof(transition));
	if (new != NULL) {
		new->next = NULL;
		new->in = in;
		new->out = out;
		new->move = m;
		new->next_state = n_s;
	}
	else {
		printf("Error: not enough memory...");
		exit(0);
	}

	if (graph[s].transitionsList == NULL) {   // insert when list is empty
		graph[s].transitionsList = new;
		new->next = NULL;
		return;
	}

	if (new->in <= graph[s].transitionsList->in) { // insert as first element
		new->next = graph[s].transitionsList;
		graph[s].transitionsList = new;
		return;
	}

	transition *curr = graph[s].transitionsList;          // insert in order (includes insertion as last)
	transition *succ = curr->next;
	while (succ != NULL && new->in > succ->in) {
		curr = succ;
		succ = succ->next;
	}

	new->next = succ;
	curr->next = new;
}

//***************************************
void run() {
    initTape();
    putInStack(tape);
    acceptString = false;
    atLeastAnUndefinedPath = false;
    executeTM(DEFAULT_PADDING_DIM, startingState, 1);
    printf("%c\n", checkComputationResult());
    //freeStack();
}

//***************************************
void executeTM(int index, int currState, unsigned int iteration) {

	if (acceptString == true) // end the computation, string already accepted
		return;

	char * currTape = stack->tape;
	int accessibleTransitions = countAccessibleTransitions(currState, currTape[index]);

	while (acceptString == false && iteration <= iterationsLimit && accessibleTransitions != 0) {

		if (DEBUG) {
			//sleep(1);
			printf("\n###############################\n");
			printf("current state: %d\n", currState);
			printf("accessible transitions: %d\n", accessibleTransitions);
			printf("iteration: %u\n", iteration);
		}

		if (accessibleTransitions >= 2) {
			performNonDeterministicTransition(currTape, currState, index, iteration);
			return;
		}
		else {
			performTransition(currTape, &currState, &index, &iteration);
		}

		if (index == -1 || index == strlen(currTape))
			currTape = reallocTape(currTape, &index);

		accessibleTransitions = countAccessibleTransitions(currState, currTape[index]);
	}

	if (graph[currState].isAccState == true) { // accept string
		acceptString = true;
		if (DEBUG) printf("ACCEPT\n");
		return;
	}
	else if (iteration > iterationsLimit) {  // undefined
		atLeastAnUndefinedPath = true;
		if (DEBUG) printf("UNDEFINED\n");
		return;
	}
	else if (accessibleTransitions == 0) {  // reject string
		if (DEBUG) printf("REJECT\n");
		return;
	}
}

//***************************************
void performTransition(char * currTape, int * state, int * i, unsigned int * it) {
	transition * p = graph[*state].transitionsList;
	bool found = false;
	while (p != NULL && found == false) {
		if (p->in == currTape[*i]) {
			if (DEBUG) printf("----------   Deterministic Transition   ----------\n");
			found = true;
			currTape[*i] = p->out;
			if (DEBUG) printf("index: %d\n", *i);
			*i = *i + p->move;
			if (DEBUG) printf("new index: %d\n", *i);
			*it = *it + 1;
			if (DEBUG) printf("iteration: %u\n", *it);
			*state = p->next_state;
			if (DEBUG) printf("new state: %d\n", *state);
		}
		p = p->next;
	}
}

//****************************************
void performNonDeterministicTransition(char * currTape, int state, int i, unsigned int it) {
	transition * p = graph[state].transitionsList;
	while (p != NULL) {
		if (p->in == currTape[i]) {
			if (DEBUG) printf("**********   NON-DETERMINISTIC TRANSITION   **********\n");
			if (DEBUG) {
				printf("index: %d\n", i);
				printf("new index: %d\n", i+p->move);
				printf("iteration: %u\n", it+1);
				printf("new state: %d\n", p->next_state);
			}
			char * newTape = (char *) malloc(strlen(currTape)+1);
			strcpy(newTape, currTape);
			newTape[i] = p->out;
			if (DEBUG) printf("new tape: %s\n", newTape);
			putInStack(newTape);
			executeTM(i+p->move, p->next_state, it+1);
			popFromStack();
		}
		p = p->next;
	}
}

/****************************************************************
* Reallocates the current tape of the Turing Machine
****************************************************************/
char * reallocTape(char * currTape, int * index) {

	int length = strlen(currTape)+1;
	char * tapeCopy;

	if (*index == -1) {
		tapeCopy = (char *) malloc(length);
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
	stack->tape = currTape;
	//printf("\nRIALLOCO VETTORE TAPE (dim = %d)\n\n", (int) strlen(currTape));
	return currTape;
}

//***************************************
void putInStack(char * string) {
    stack_node * new = (stack_node *) malloc(sizeof(stack_node));
    if (string == NULL || new == NULL)
        return;
    new->tape = string;
    new->next = stack;
    stack = new;

    if (DEBUG) {
        printf("Added tape in stack\n");
        printStack();
    }

}

//***************************************
void popFromStack() {
    if (stack == NULL) {
        if (DEBUG) printf("Stack already empty\n");
        return;
    }
    stack_node * p = stack;
    stack = stack->next;
	free(p->tape);
    free(p);
    if (DEBUG) {
        printf("Removed tape from stack\n");
        printStack();
    }
}

//****************************************************************
int countAccessibleTransitions(int state, char c) {
	transition * p = graph[state].transitionsList;
	int i = 0;
	while (p != NULL) {
		if (p->in == c)
			i++;
		p = p->next;
	}
	return i;
}

/*************************************************************************
 * Initializes tape with BLANK charaters before and after the input string
 *************************************************************************/
void initTape() {
	int length = (int) strlen(inputString);
	tape = (char *) malloc(length + 2*DEFAULT_PADDING_DIM + 1);
	//tape_dim = length + 2*DEFAULT_PADDING_DIM;
	for (int i = 0; i < DEFAULT_PADDING_DIM; i++)
		tape[i] = BLANK;
	strcpy(&tape[DEFAULT_PADDING_DIM], inputString);
	for (int i = DEFAULT_PADDING_DIM+length; i < length + 2*DEFAULT_PADDING_DIM; i++)
		tape[i] = BLANK;
	tape[length+2*DEFAULT_PADDING_DIM-1] = '\0';
}

/***************************************************************************
 * Calculates the correct computation results according to the results list
 ***************************************************************************/
char checkComputationResult() {
	if (acceptString == true)             // at least a path accpets the string ---> ACCEPT STRING
        return ACCEPT;
	if (atLeastAnUndefinedPath == true)   // at least an undefined value ---> UNDEFINED
		return UNDEFINED;
	return REJECT;                        // only rejection values ---> REJECT STRING
}

/*****************************************************************
 * Frees the Turing Machine graph
 *****************************************************************/
void freeGraph() {
	int i = 0;
	transition * prec = NULL;
	transition * succ = NULL;
	while (i < states_dim) {
		if (graph[i].transitionsList != NULL) {
			prec = graph[i].transitionsList;
			succ = graph[i].transitionsList;
			while (succ != NULL) {
				succ = succ->next;
				free(prec);
				prec = succ;
			}
		}
		i++;
	}
	free(graph);
}

/*****************************************************************
 * Frees the tapes stack
 *****************************************************************/
 void freeStack() {
     while(stack != NULL) {
        popFromStack();
     }
 }

/****************************************************************
 * Displays the transitions graph
 ****************************************************************/
void printGraph() {
	transition * p;
	for (int i = 0; i < states_num; i++) {
		if (graph[i].transitionsList != NULL) {
			p = graph[i].transitionsList;
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

/****************************************************************
 * Displays the current state of the tapes' stack
 ****************************************************************/
void printStack() {
    stack_node * p = stack;
    int i = 0;
    printf("STACK:\n");
    while (p != NULL) {
        printf("%d: %s\n", i, p->tape);
        p = p->next;
		i++;
    }
    free(p);
    printf("\n\n");
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
  	printf("\nIterations limit: %u", iterationsLimit);
  	printGraph();
  	printf("\n");
  }

	readInputStrings();

    #ifdef EVAL
        freeGraph();
    #endif

	return 0;
}
