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

#define OK   			 1
#define NO               0
#define UNDEF			 2
#define RIGHT 			'R'
#define LEFT  			'L'
#define STOP  			'S'
#define BLANK 			'_'
#define TERM_CHAR 		'@'
#define INPUT_DIM       500
#define TAPE_DIM        500
#define STARTING_INDEX  200

void initGraph();
void initTape();
void readFile();
void insertTransitionInGraph(int, char, char, char, int);
void constructTrasitionsTree();
void execute(char *, int, int, int);
int nextIndex(int, char);
void printGraph();
void printTape(int, char *);
void freeResultsList();
void insertResult(int);
void printResults();
int checkComputationResult();

typedef enum {true, false} bool;

typedef struct r {
	int value;
	struct r * next;
} res_type;

typedef struct s {     // node of the graph
	char in;
	char out;
	char move;
	int next_state;
	struct s * next;   // list of all possible next states of the current one
} state;

state ** states;       		// list containing all the states read from input
res_type * resList;				// list containing all computation results (OK, NO or UNDEF)
int states_size = 30;
int states_num = 0;       		// the number of states of the TM
int accState;	     	  		// all passible acceptation states
int reject_state = -1;    		// the state of rejection
int iterationsLimit;            // the limit to the iteration number (to avoid machine loop)
char inputString[INPUT_DIM];     // current input string on the machine tape
int length;
char tape[TAPE_DIM];


/**************************************************************
 * Main function
 **************************************************************/
int main(int argc, char *argv[]) {
	states = (state **) malloc (states_size * sizeof(state));
	initGraph();
	readFile();
	
	//printf("\nStates number: %d", states_num);
	//printf("\nAcceptation state: %d", accState);
	//printf("\nIterations limit: %d", iterationsLimit);
	
	//printGraph();
	
	int res = scanf("%s", inputString);
	//printf("\n");
	while (res != EOF) {
		initTape();
		execute(tape, STARTING_INDEX, 0, 1);
		int result = checkComputationResult();
		if (result == UNDEF)
			printf("U\n");
		else
			printf("%d\n", result);
		freeResultsList();
		res = scanf("%s", inputString);
	}
}

/***************************************************
* Initializes graph (states array) elements to NULL
****************************************************/
void initGraph() {
	for (int i = 0; i < states_size; i++)
		states[i] = NULL;
}

/*************************************************************************
 * Initializes tape with BLANK charaters before and after the input string
 *************************************************************************/
void initTape() {
	length = (int) strlen(inputString);
	for (int i = 0; i < STARTING_INDEX; i++)
		tape[i] = BLANK;
	strcpy(&tape[STARTING_INDEX], inputString);
	for (int i = STARTING_INDEX+length; i < TAPE_DIM; i++)
		tape[i] = BLANK;
	tape[TAPE_DIM-1] = '\0';
}

/***************************************************************
 * Reads the file saving data in program structures:
 * => reads only the lines that contain transitions
 ***************************************************************/
void readFile() {
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
	while (strcmp(in, "acc") != 0) { // cycle until find the word "acc"
		sscanf(in, "%d", &s); 		 // read state
		if (s >= states_num)		 // maintain the maximum state number
			states_num = s+1;
		scanf("%s", in);
		inputChar = in[0];    		 // read input character
		scanf("%s", in);
		outputChar = in[0];   		 // read output character
		scanf("%s", in);
		move = in[0];		  		 		 // read move
		scanf("%s", in);
		sscanf(in, "%d", &next_s); // read next state
		insertTransitionInGraph(s, inputChar, outputChar, move, next_s);
		scanf("%s", in);
	}
	
	int i = 0;
	while (strcmp(in, "max") != 0 ) { // read acceptation states
		scanf("%s", in);
		sscanf(in, "%d", &accState);
		i++;
	}
	
	scanf("%d", &iterationsLimit);    // read maximum number of iterations
	
	scanf("%s", in);                  // read the word "run" to start computations
	if (strcmp(in, "run") != 0)
		exit(0);
}

/*****************************************************************
 * Inserts the transition read from stdin in the transitions graph
 *****************************************************************/

// NOTA: se aggiungessi sempre in testa nelle liste avrei costruzione del grafo in tempo costante

void insertTransitionInGraph(int s, char in, char out, char m, int n_s) {
	state * new = (state *) malloc(sizeof(state *));
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
	
	if (states[s] == NULL) {   // insert when list is empty
		states[s] = new;
		new->next = NULL;
		return;
	}
	
	if (new->in <= states[s]->in) { // insert as first element
		new->next = states[s];
		states[s] = new;
		return;
	}
	
	state *curr = states[s];          // insert in order (includes insertion as last)
	state *succ = curr->next;
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
 * Inserts a node in the results list
 ****************************************************************/
void insertResult(int value) {
	
	res_type * new = (res_type *) malloc(sizeof(res_type *));
	
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
}


/****************************************************************
* Executes the Turing Machine
*****************************************************************/
void execute(char * tape, int index, int curr_state, int iteration) {
	
	if (curr_state == accState) {
		/*printf("\n\n******************************************************");
		printf("\n********************** ACCEPTED **********************");
		printf("\n******************************************************");*/
		insertResult(OK);
		return;
	}

	else if (iteration > iterationsLimit) {
		/*printf("\n\n******************************************************");
		printf("\n********************** UNDEFINED *********************");
		printf("\n******************************************************");*/
		insertResult(UNDEF);
		return;
	}
	
	else if (curr_state == reject_state || states[curr_state] == NULL) {
		/*printf("\n\n******************************************************");
		printf("\n********************** REJECTED **********************");
		printf("\n******************************************************");*/
		insertResult(NO);
		return;
	}
		
	char newTape[TAPE_DIM];
	state * p = states[curr_state];
	bool reject = true;
	
	while (p != NULL) {
		if (p->in == tape[index]) {
			reject = false;
			strcpy(newTape, tape);
			newTape[index] = p->out;
			
			//printf("\n%d %c %c %c %d -> iteration: %d", curr_state, p->in, p->out, p->move, p->next_state, iteration);
			//printf("\n%s", &tape[STARTING_INDEX]);
			//printf("\n%s", &newTape[STARTING_INDEX]);
			//printTape(index, newTape);
			//printf("\nnext index: %d", nextIndex(index, p->move));
			
			execute(newTape, nextIndex(index, p->move), p->next_state, iteration+1);
		}
		p = p->next;
	}
	
	if (reject == true)
		execute(newTape, -1, reject_state, -1);
	
	return;
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

/**************************************************************************
 * Calculates the correct computation results according to the results list
 **************************************************************************/
int checkComputationResult() {
	res_type * p = resList;
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
 * Calculates the correct computation results according to the results list
 **************************************************************************/
void freeResultsList() {
	res_type * prec = resList->next;
	res_type * succ = resList->next;
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
	for (int i = 0; i < states_num; i++) {
		if (states[i] != NULL) {
			state *p = states[i];
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
	printf("\n");
	for (int j = 0; j < length; j++)
		printf("%c", t[STARTING_INDEX+j]);
	printf("\n");
	for (int j = STARTING_INDEX; j < i; j++)
		printf("%c", ' ');
	printf("∆\n");
}

/****************************************************************
 * Displays the current results list
 ****************************************************************/
void printResults() {
	res_type * p = resList;
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