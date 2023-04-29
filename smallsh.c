#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void prompt();
void insertPID(char* string);

/**********************************************************************************
    ** Description: 
    ** Parameters: 
**********************************************************************************/
void prompt()
{
    char* line = NULL;
    size_t buffer = 0;
    char userInput[2048];
    memset(userInput, '\0', 2048);
    
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
    printf("%s\n", userInput);


    free(line);
    line = NULL;
}

/**********************************************************************************
    ** Description: takes a string and replaces all instances of "$$" with the process
    ID of the small shell
    ** Parameters: 
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
