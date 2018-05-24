#include <stdio.h>
#include <string.h>

void readTransitions();
void constructTrasitionsTree();
void readAccStates();
void readTransitionsNumberLimit();
void readInputString();
void execute();

typdef enum { true, false } bool;
typdef enum { L, S, R} direction;

typedef struct s {
	int num;
	char in;
	char out;
	direction dir;
	int next[10];   // all possible next states
} state;

state states[30];   // array containing all the states read from input

int main(int argc, char *argv[]) {
	
	char in[10];
	
	while (strcmp(in, "run") != 0) {
		scanf ("%[^\n]%*c", in);     // reads input string with space characters
		printf("OK -> %s\n", in);
	}
		
	
}