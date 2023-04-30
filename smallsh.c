#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void prompt();
void insertPID(char* string);
void argsCreate(char* input, char* args[]);
void changeDir(char* args[]);


/**********************************************************************************
    ** Description: 
    ** Parameters: 
**********************************************************************************/
void prompt()
{
    char* line = NULL; //for user input in getline
    size_t buffer = 0; 
    char userInput[2048]; //max size of user input
    memset(userInput, '\0', sizeof(userInput));
    char* args[512]; //max of 512 arguments per line
    memset(args, 0, sizeof(args));
    
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
    free(line);
    line == NULL;
    //get rid of newline character from user's input
    userInput[strlen(userInput) - 1] = '\0';

    //if $$ is in user input, find and replace $$'s with PID
    insertPID(userInput);

    //create argument array from user input
    argsCreate(userInput, args);


    //if user wants to clear screen
    if(strcmp(args[0], "clear") == 0)
    {
        printf("\033[2J\033[1;1H");
        //or system("clear");
    }
    //if user wants to exit smallsh
    if(strcmp(args[0], "exit") == 0)
    {
        exit(0);
    }
    //if user wants to change directory
    else if(strcmp(args[0], "cd") == 0)
    {
        changeDir(args);
    }
    //if user wants to print current directory
    else if(strcmp(args[0], "pwd") == 0)
    {
        char currDir[2048];
        memset(currDir, '\0', sizeof(currDir));
        getcwd(currDir, sizeof(currDir));
        printf("%s\n", currDir);
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
    ** Description: 
    ** Parameters: 
**********************************************************************************/
void changeDir(char* args[])
{
    //obtain home and current directories
    char* homeDir = getenv("HOME");
    char currDir[2048];
    memset(currDir, '\0', sizeof(currDir));
    getcwd(currDir, sizeof(currDir));

    //no second argument was provided. navigate to HOME directory
    if(args[1] == NULL)
    {
        //print current directory
        memset(currDir, '\0', sizeof(currDir));
        getcwd(currDir, sizeof(currDir));
        printf("currDir: \"%s\"\n", currDir);

        //alert
        printf("no directory provided. navigating to HOME directory called \"%s\".\n", homeDir);

        //move to home directory
        chdir(homeDir);

        //print current directory after move
        memset(currDir, '\0', sizeof(currDir));
        getcwd(currDir, sizeof(currDir));
        printf("currDir after move: \"%s\"\n", currDir);
        
    }

    //path to navigate to was provided
    else
    {
        /* ABSOULTE PATHS:
            if there is a '/' in the beginning, it is an absolute path
            and takes the form of "/nfs/stak/users/rosenaum/..."
        */
        /* RELATIVE PATHS:
            if there is no '/' in the beginning, or if it starts with a '.' or a ".."
            it is a relative path.
        */
       
        //print current directory
        memset(currDir, '\0', sizeof(currDir));
        getcwd(currDir, sizeof(currDir));
        printf("currDir: \"%s\"\n", currDir);

        //alert
        printf("navigating to directory \"%s\".\n", args[1]);

        //move to directory
        chdir(args[1]);

        //print current directory after move
        memset(currDir, '\0', sizeof(currDir));
        getcwd(currDir, sizeof(currDir));
        printf("currDir after move: \"%s\"\n", currDir);
    }
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
