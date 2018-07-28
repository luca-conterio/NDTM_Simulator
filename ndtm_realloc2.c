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

#define OK   			            '1'
#define NO                    '0'
#define UNDEF			            'U'
#define RIGHT 			          'R'
#define LEFT  			          'L'
#define STOP  			          'S'
#define BLANK 			          '_'
#define TERM_CHAR 		        '@'
#define DEFAULT_TAPE_DIM      250
#define DEFAULT_PADDING_DIM   50
#define DEFAULT_INPUT_DIM     250
#define DEFAULT_STATES_DIM    25

typedef enum {true, false} bool;

typedef struct s {     // node of the graph
	char in;
	char out;
	char move;
	int next_state;
	struct s * next;   // list of all possible next states of the current one
} transition;

typedef struct r {
	int value;
	struct r * next;
} int_node;

void initGraph();
void initTape();
void readMTStructure();
void readInputString();
void run();
void addAcceptationState(int);
void insertTransitionInGraph(int, char, char, char, int);
void printGraph();
void printTape(int, char *);
void execute(char *, int, int, int);
bool checkIfAccState(int);
int nextIndex(int, char);
char * reallocTape(int *, char *);
void freeResultsList();
void insertResult(int);
void printResults();
int checkComputationResult();

int states_num = 0;											// the number of states of the TM
int states_dim = DEFAULT_STATES_DIM;    // the actual size of states array (graph)
transition ** graph;				    				// array containing all the states read from input

int_node * accStates = NULL;	     	    // acceptation states list
int reject_state = -1;    							// the state of rejection
int iterationsLimit;            				// the limit to the iteration number (to avoid machine loop)

int tape_dim = DEFAULT_TAPE_DIM;        // the current tape length
char * tape;                            // the tape of the Turing Machine

int input_dim;
char * inputString;

int_node * resList = NULL;							// list containing all computation results (OK, NO or UNDEF)

/**************************************************************
 * 										Main function
 **************************************************************/
int main(int argc, char *argv[]) {
	graph = (transition **) malloc(DEFAULT_STATES_DIM * sizeof(transition));
	initGraph();
	readMTStructure();

/*	printf("\nStates number: %d", states_num);
	printf("\nAcceptation states: ");
	int_node * p = accStates;
	while (p != NULL) {
		printf("%d ", p->value);
		p = p->next;
	}
	printf("\nIterations limit: %d", iterationsLimit);
	printGraph();
	printf("\n"); */
	readInputString();
}

/***************************************************************
 * Initializes graph (states vector) elements to NULL
 ***************************************************************/
void initGraph() {
	for(int i = 0; i < DEFAULT_STATES_DIM; i++)
		graph[i] = NULL;
}

/*******************************************************************
 * Reads the stind (MT structure) saving data in program structures
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
		sscanf(in, "%d", &s); 		 			// read state
		scanf("%s", in);
		inputChar = in[0];    		 			// read input character
		scanf("%s", in);
		outputChar = in[0];   		 			// read output character
		scanf("%s", in);
		move = in[0];		  		 					// read move
		scanf("%s", in);
		sscanf(in, "%d", &next_s);   		// read next state
		if (s >= states_num || next_s >= states_num) {	 				// maintain the maximum state number
			if (s >= next_s)
				states_num = s + 1;
			else
				states_num = next_s + 1;
		}
		if (states_num >= states_dim) {
			graph = realloc(graph, 2 * states_num * sizeof(transition));
			for (int i = states_dim; i < states_num * 2; i++) // re-initialize states vector (graph) with NULL in new positions
				graph[i] = NULL;
			states_dim = states_num * 2;
			printf("\nRIALLOCO VETTORE STATI (dim = %d)\n\n", states_dim);
		}
		insertTransitionInGraph(s, inputChar, outputChar, move, next_s);
		scanf("%s", in);
	}

	int accState;
	scanf("%s", in);
	while (strcmp(in, "max") != 0) { 	// read acceptation states
		sscanf(in, "%d", &accState);
		addAcceptationState(accState);
		scanf("%s", in);
	}

	scanf("%d", &iterationsLimit);    // read maximum number of iterations

	scanf("%s", in);                  // read the word "run" to start computations
	if (strcmp(in, "run") != 0)
		exit(0);
	else
		getchar(); // consume '\n' character after "run" string
}

/*******************************************************************
 * Insert a new integer in accStates list (acceptation states list)
 *******************************************************************/
 void addAcceptationState(int s) {
	 int_node * new = (int_node *) malloc(sizeof(int_node *));
	 if (new != NULL) {
		 new->value = s;
		 new->next = NULL;
	 }
	 else {
		 printf("Error: not enough memory...");
		 exit(0);
	 }
	 new->next = accStates;
	 accStates = new;
	 new = NULL;
 	 free(new);
 }

/****************************************************************
 * Reads the next input string from stdin
 ****************************************************************/
void readInputString() {

	input_dim = DEFAULT_INPUT_DIM; 																			// actual size of the input string
	inputString = (char *) malloc(DEFAULT_INPUT_DIM * sizeof(char)); 		// current input string

	int i = 0;
	char c = ' ';

	while (c != EOF) {
		c = getchar();
		if (i == input_dim && c != EOF) {
			inputString = realloc(inputString, 4 * input_dim * sizeof(char));
			input_dim = input_dim * 4;
			printf("\nRIALLOCO VETTORE INPUT (dim = %d)\n\n", input_dim);
		}
		if (c == '\n') {
			inputString[i] = '\0';
			//printf("input string: %s\n", inputString);
			i = 0;
			run();
		}
		else if (c != EOF) {
			inputString[i] = c;
			i++;
		}
	}
}

/*************************************************************************
 * Runs the actual computation, calculating the result
 *************************************************************************/
void run() {
	initTape();
	execute(tape, DEFAULT_PADDING_DIM, 0, 1);
	printf("%c\n", checkComputationResult());
	//freeTape();
	freeResultsList();
}

/*************************************************************************
 * Initializes tape with BLANK charaters before and after the input string
 *************************************************************************/
void initTape() {
	int length = (int) strlen(inputString);
	tape = (char *) malloc((length + 2*DEFAULT_PADDING_DIM) * sizeof(char));
	tape_dim = length + 2*DEFAULT_PADDING_DIM;
	for (int i = 0; i < DEFAULT_PADDING_DIM; i++)
		tape[i] = BLANK;
	strcpy(&tape[DEFAULT_PADDING_DIM], inputString);
	for (int i = DEFAULT_PADDING_DIM+length; i < length + 2*DEFAULT_PADDING_DIM; i++)
		tape[i] = BLANK;
	tape[length+2*DEFAULT_PADDING_DIM-1] = '\0';
}

/******************************************************************
 * Inserts the transition read from stdin in the transitions graph
 ******************************************************************/

void insertTransitionInGraph(int s, char in, char out, char m, int n_s) {

	transition * new = (transition *) malloc(sizeof(transition *));
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

	if (graph[s] == NULL) {   // insert when list is empty
		graph[s] = new;
		new->next = NULL;
		return;
	}

	if (new->in <= graph[s]->in) { // insert as first element
		new->next = graph[s];
		graph[s] = new;
		return;
	}

	transition *curr = graph[s];          // insert in order (includes insertion as last)
	transition *succ = curr->next;
	while (succ != NULL && new->in > succ->in) {
		curr = succ;
		succ = succ->next;
	}

	new->next = succ;
	curr->next = new;

	new = NULL;
	free(new);

	return;
}

/****************************************************************
* Executes the Turing Machine
*****************************************************************/
void execute(char * currTape, int index, int curr_state, int iteration) {

  if (checkIfAccState(curr_state) == true) {
    insertResult(OK);
    return;
  }
  else if (iteration > iterationsLimit) {
    insertResult(UNDEF);
    return;
  }

  transition * p = graph[curr_state];
  char * newTape;
  int next_index;
  bool reject = true;

  while (p != NULL) {
    if (p->in == currTape[index]) {
      reject = false;

      if (index == 0 || index == strlen(currTape)) {
        currTape = reallocTape(&index, currTape);
      }

      newTape = (char *) malloc((strlen(currTape)+1) * sizeof(char));
      strcpy(newTape, currTape);
      newTape[index] = p->out;
      next_index = nextIndex(index, p->move);
      //printf("%d %c %c %c %d -> iteration: %d\n", curr_state, p->in, p->out, p->move, p->next_state, iteration);
      //printTape(index, newTape);
      execute(newTape, next_index, p->next_state, iteration+1);
    }
    p = p->next;
  }

  if (reject == true)
    insertResult(NO);

  return;
}

/***********************************************************************
 * Checks if the current state is an acceptation state
 ***********************************************************************/
bool checkIfAccState(int state) {
	int_node * p = accStates;
	while (p != NULL) {
		if (p->value == state)
			return true;
		p = p->next;
	}
	return false;
}

/***********************************************************************
 * Returns the next index in the tape, according to the transition move
 ***********************************************************************/
int nextIndex(int i, char m) {
	if (m == RIGHT)
		return i+1;
	else if (m == LEFT)
		return i-1;
	else if (m == STOP)
		return i;
	return -1;
}

/****************************************************************
* Reallocates the current tape of the Turing Machine
****************************************************************/
char * reallocTape(int * index, char * currTape) {
	int length = strlen(currTape)+1;
	char * tapeCopy = (char *) malloc(length * sizeof(char));
	currTape = realloc(currTape, (length + 2*DEFAULT_PADDING_DIM) * sizeof(char));
	strcpy(tapeCopy, currTape);
	for (int i = 0; i < DEFAULT_PADDING_DIM; i++)
		currTape[i] = BLANK;
	strcpy(&currTape[DEFAULT_PADDING_DIM], tapeCopy);
	for (int i = DEFAULT_PADDING_DIM+strlen(tapeCopy); i < length+2*DEFAULT_PADDING_DIM; i++)
		currTape[i] = BLANK;
	currTape[length+2*DEFAULT_PADDING_DIM-1] = '\0';
	*index = *index + DEFAULT_PADDING_DIM;
	//printf("\nRIALLOCO VETTORE TAPE (dim = %d)\n\n", (int) strlen(currTape));
	//printTape(index + DEFAULT_PADDING_DIM, currTape);
	return currTape;
}


/****************************************************************
 * Inserts a node in the results list
 ****************************************************************/
void insertResult(int value) {
	int_node * new = (int_node *) malloc(sizeof(int_node *));
	if (new != NULL) {
		new->value = value;
		new->next = NULL;
	}
	else {
		printf("Error: not enough memory...");
		exit(0);
	}
	new->next = resList;
	resList = new;

	new = NULL;
	free(new);
}

/**************************************************************************
 * Calculates the correct computation results according to the results list
 **************************************************************************/
int checkComputationResult() {
	int_node * p = resList;
	bool trovUNDEF = false;
	//printResults();
	while (p != NULL) {
		if (p->value == OK)
			return OK;       // at least an acceptation ---> ACCEPT STRING
		else if (p->value == UNDEF)
			trovUNDEF = true;
		p = p->next;
	}
	if (trovUNDEF == true)  // at least an undefined value ---> UNDEFINED
		return UNDEF;
	return NO;              // only rejection values ---> REJECT STRING
}

/**************************************************************************
 * Frees the results list
 **************************************************************************/
void freeResultsList() {
	int_node * prec = resList;
	int_node * succ = resList;
	while (succ != NULL) {
		succ = succ->next;
		free(prec);
		prec = succ;
	}
	resList = NULL;
}

/****************************************************************
 * Displays the transitions graph
 ****************************************************************/
void printGraph() {
	transition * p;
	for (int i = 0; i < states_num; i++) {
		if (graph[i] != NULL) {
			p = graph[i];
			while (p != NULL) {
				printf("\n%d %c %c %c %d", i, p->in, p->out, p->move, p->next_state);
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
 * Displays the current results list
 ****************************************************************/
void printResults() {
	int_node * p = resList;
	printf("\nRESULTS: ");
	while (p != NULL) {
		if (p->value == OK)
			printf(" 1");
		else if (p->value == NO)
			printf(" 0");
		else if (p->value == UNDEF)
			printf(" U");
		p = p->next;
	}
	printf("\n");
}
