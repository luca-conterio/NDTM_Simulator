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

#define TRUE   1
#define FALSE  0
#define RIGHT 'R';
#define LEFT  'L';
#define STOP  'S';
#define BLANK '_';

void initGraph();
void readFile();
void insertTransitionInGraph(int, char, char, char, int);
void constructTrasitionsTree();
void readAccStates();
void readTransitionsNumberLimit();
void readInputString();
void execute();
void printGraph();

typedef struct s {     // node of the graph
	char in;
	char out;
	char move;
	int next_state;
	struct s * next;   // list of all possible next states of the current one
} state;

state * states[30];    // list containing all the states read from input
int states_num = 0;    // the number of states of the TM
int accStates[];	   // all passible acceptation states
int iterationLimit;	   // the limit to the iteration number (to avoid machine loop)
char inputString[100];

typedef struct tn {    // node of the computation tree
	int state;
	char current_input[];
	int index;
	int height;
	char in;
	char out;
	char move;
} treeNode;

int main(int argc, char *argv[]) {
	initGraph();
	readFile();

	printf("\nStates number: %d", states_num);
	printGraph();
}

void initGraph() {
	for(int i = 0; i < 50; i++)
		states[i] = NULL;
}

/***************************************************************
* Reads the file saving data in program structures:
* => reads only the lines that contain transitions
****************************************************************/
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
		if (s >= states_num)		 // mantain the maximum state number
			states_num = s+1;
		scanf("%s", in);
		inputChar = in[0];    		 // read input character
		scanf("%s", in);
		outputChar = in[0];   		 // read output character
		scanf("%s", in);
		move = in[0];		  		 // read move
		scanf("%s", in);
		sscanf(in, "%d", &next_s);  // read next state
		insertTransitionInGraph(s, inputChar, outputChar, move, next_s);
		//printf("OK -> %d%c%c%c%d\n", s, inputChar, outputChar, move, next_s);
		scanf("%s", in);
	}
}

/****************************************************************
* Inserts the transition read from stdin in the transitions graph
*****************************************************************/
void insertTransitionInGraph(int s, char in, char out, char m, int n_s) {
	state * new = (state *) malloc(sizeof(state *));
	if (new != NULL) {
		printf("0");
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
	
	if (states[s] == NULL) {
		states[s] = new;
		new->next = NULL;
		return;
	}
	
	state *p = states[s];
	while (p->next != NULL)
		p = p->next;
	p->next = new;
	return;
}

/****************************************************************
* Displays the transitions graph
*****************************************************************/
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














