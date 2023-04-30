#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void prompt();
void insertPID(char* string);
void argsCreate(char* input, char* args[]);


/**********************************************************************************
    ** Description: 
    ** Parameters: 
**********************************************************************************/
void prompt()
{
    char* line = NULL; //for user input in getline
    size_t buffer = 0; 
    char userInput[2048]; //max size of user input
    memset(userInput, '\0', 2048);
    char* args[512]; //max of 512 arguments per line
    memset(args, 0, 512);
    
    //prompt for and obtain user input
    printf(": ");
    getline(&line, &buffer, stdin);
    
    //if a comment or empty line, reprompt
    if(line[0] == '#' || line[0] == '\n')
    {
        return;
    }

    //copy user input to max sized string of null terminators
    strcpy(userInput, line);
    //get rid of newline character from user's input
    userInput[strlen(userInput) - 1] = '\0';

    //if $$ is in user input, find and replace $$'s with PID
    insertPID(userInput);

    //create argument array from user input
    argsCreate(userInput, args);

    //if user input is "exit", exit the program
    if(strcmp(args[0], "exit") == 0)
    {
        exit(0);
    }

    free(line);
    line = NULL;
}

/**********************************************************************************
    ** Description: takes the string the user input on the command line, and tokenizes
    into an array of arguments
    ** Parameters: string input from user, array of arguments to be filled & updated
**********************************************************************************/
void argsCreate(char* input, char* args[])
{
    char* saveptr;
    char* token;

    // get first token
    token = strtok_r(input, " ", &saveptr);

    int j = 0;
    // walk through other tokens
    while (token != NULL)
    {
        args[j] = token;
        j++;
        token = strtok_r(NULL, " ", &saveptr);
    }

    //debug: print out all arguments in array
    j = 0;
    while(args[j] != NULL)
    {
        printf("Arg %d: %s\n", j, args[j]);
        j++;
    }
}


/**********************************************************************************
    ** Description: takes a string and replaces all instances of "$$" with the process
    ID of the small shell
    ** Parameters: string to insert PID into
**********************************************************************************/
void insertPID(char* string){

    //base case if string has no consecutive dollar signs
    if(strstr(string, "$$") == NULL){
        return;
    }

    for (int i = 0; i < strlen(string); i ++)
    {
        //if "$$" is found and not beyond end of string
        if ( (string[i] == '$')  && (string[i + 1] == '$') && (i + 1 < strlen(string)))
        {
            //duplicate string, and replace "$$" with "%d"
            char * temp = strdup(string);
            temp[i] = '%';
            temp[i + 1] = 'd';

            //format the pid into the %d, and then copy back into string
            sprintf(string, temp, getpid());
            free(temp);
        }
    }

    //recursive call to check for more $$ in string
    insertPID(string);
}

int main()
{
    while(1)
    {
        fflush(stdout);
        fflush(stdin);
        prompt();
    }
}
