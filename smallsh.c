#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

struct redirect
{
    int inBool;
    int outBool;
    char inFile[128];
    char outFile[128];
};


void prompt(int* exitStatus);
void insertPID(char* string);
void argsCreate(char* input, char* args[], int* numArgs);
void changeDir(char* args[]);
struct redirect checkRedirect(char* args[], int numArgs);


/**********************************************************************************
    ** Description: 
    ** Parameters: 
**********************************************************************************/
void prompt(int* exitStatus)
{
    char* line = NULL; //for user input in getline
    size_t buffer = 0; 
    char userInput[2048]; //max size of user input
    memset(userInput, '\0', sizeof(userInput));
    char* args[512]; //max of 512 arguments per line
    memset(args, 0, sizeof(args));
    int numArgs = -1; //number of arguments
    
    //prompt for and obtain user input
    printf("\n: ");
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
    argsCreate(userInput, args, &numArgs);

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
    //status
    else if(strcmp(args[0], "status") == 0)
    {
        printf("exit value %d\n", *exitStatus);
    }

    //else, fork and execute command
    else
    {
        //fork a child process
        pid_t spawnPid = -5;
        spawnPid = fork();

        switch(spawnPid)
        {
            //fork failed
            case -1:
            {
                perror("Hull Breach!\n");
                exit(1);
            }

            //in child process
            case 0:
            {
                //obtain any redirection, store in struct
                struct redirect redirStatus = checkRedirect(args, numArgs);
                //test
                fflush(stdout);
                // printf("Redirection Status\nInput: %d\nSource: \"%s\"\nOutput: %d\nDestination: \"%s\"\n", redirStatus.inBool, redirStatus.inFile, redirStatus.outBool, redirStatus.outFile);

                //if redirecting input
                if(redirStatus.inBool)
                {
                    //open source file
                    int sourceFD = open(redirStatus.inFile, O_RDONLY);
                    if(sourceFD == -1){ //cannot open file
                        perror("Input File"); 
                        exit(1); 
                    }
                    //debug/test
                    fflush(stdout);
                    printf("sourceFD == %d\n", sourceFD); 

                    //redirect stdin to this source file descriptor
                    int result = dup2(sourceFD, 0);
                    if(result == -1){ 
                        perror("source dup2()"); 
                        exit(2); 
                    }
                }

                //if redirecting output
                if(redirStatus.outBool)
                {
                    //open destination file
                    int destinationFD = open(redirStatus.outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (destinationFD == -1) { 
                        perror("Output File"); 
                        exit(1); 
                    }
                    printf("destinationFD == %d\n", destinationFD); // Written to terminal
                
                    //redirect stdout to this destination file
                    int result = dup2(destinationFD, 1);
                    if (result == -1) { 
                        perror("target dup2()"); 
                        exit(2); 
                    }
                }
            
                //execute command
                if (execvp(args[0], args) == -1) {
                    //if execvp returns -1, command command was not found
                    printf("Command not found: %s\n", args[0]);
                    exit(1);
                }
                exit(1);
            }

            //in parent process
            default:
            {
                //wait for child process to finish
                int childExitMethod;
                waitpid(spawnPid, &childExitMethod, 0);

                //check exit status
                if(WIFSIGNALED(childExitMethod) != 0) //non zero if terminated by signal
                {
                    int termSignal = WTERMSIG(childExitMethod);
                    printf("terminated by signal %d\n", termSignal);
                }
                else if(WIFEXITED(childExitMethod) != 0) //non zero of terminated normally
                {
                    *exitStatus = WEXITSTATUS(childExitMethod);
                    // printf("The process exited normally! It's exit status was: %d\n", exitStatus);
                }                
            }
        }
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

    for(int i = 0; i < strlen(string); i++)
    {
        //if "$$" is found and not beyond end of string
        if((string[i] == '$') && (string[i + 1] == '$') && (i + 1 < strlen(string)))
        {
            //duplicate string, and replace "$$" with "%d"
            char* dupe = strdup(string);
            dupe[i] = '%';
            dupe[i + 1] = 'd';

            //format the pid into the %d, and then copy back into string
            sprintf(string, dupe, getpid());
            free(dupe);
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
void argsCreate(char* input, char* args[], int* numArgs)
{
    char* saveptr;
    char* token;

    //get first token
    token = strtok_r(input, " ", &saveptr);

    int j = 0;
    //walk through other tokens
    while(token != NULL)
    {
        args[j] = token;
        j++;
        token = strtok_r(NULL, " ", &saveptr);
    }

    //update number of arguments
    *numArgs = j;
}

/**********************************************************************************
    ** Description: changes the current working directory to the directory specified,
    and if no directory is specified, changes to the HOME directory
    ** Parameters: arguments array
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
        // //print current directory
        // memset(currDir, '\0', sizeof(currDir));
        // getcwd(currDir, sizeof(currDir));
        // printf("currDir: \"%s\"\n", currDir);

        // //alert
        // printf("no directory provided. navigating to HOME directory called \"%s\".\n", homeDir);

        //move to home directory
        chdir(homeDir);

        // //print current directory after move
        // memset(currDir, '\0', sizeof(currDir));
        // getcwd(currDir, sizeof(currDir));
        // printf("currDir after move: \"%s\"\n", currDir);
        
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
       
        // //print current directory
        // memset(currDir, '\0', sizeof(currDir));
        // getcwd(currDir, sizeof(currDir));
        // printf("currDir: \"%s\"\n", currDir);

        // //alert
        // printf("navigating to directory \"%s\".\n", args[1]);

        //move to directory, print corresponding error if chdir fails
        if(chdir(args[1]) == -1) 
        {
            perror("chdir");
        }

        // //print current directory after move
        // memset(currDir, '\0', sizeof(currDir));
        // getcwd(currDir, sizeof(currDir));
        // printf("currDir after move: \"%s\"\n", currDir);
    }
}

/**********************************************************************************
    ** Description: 
    ** Parameters: 
**********************************************************************************/
struct redirect checkRedirect(char* args[], int numArgs){
    //initialize struct to be returned
    struct redirect redirStatus;
    redirStatus.inBool = 0, redirStatus.outBool = 0;
    memset(redirStatus.inFile, '\0', sizeof(redirStatus.inFile));
    memset(redirStatus.outFile, '\0', sizeof(redirStatus.outFile));

    //loop through args array, looking for redirection
    for(int i = 0; i < numArgs; i++)
    {
        //if input redirection "<"
        if(strcmp(args[i], "<") == 0)
        {
            //trigger inBool and remove symbol as an argument
            redirStatus.inBool = 1;
            args[i] = NULL;

            //check if there is a filename after that symbol
            if(args[i + 1] != NULL)
            {
                //copy filename into struct and remove filename as an argument
                strcpy(redirStatus.inFile, args[i + 1]);
                args[i + 1] = NULL;
                i++;
            }
            else
            {
                fflush(stdout);
                printf("No filename provided after redirection symbol.\n");
            }
        }

        //if output redirection ">"
        else if(strcmp(args[i], ">") == 0)
        {
            //trigger outBool and remove symbol as an argument
            redirStatus.outBool = 1;
            args[i] = NULL;

            //check if there is a filename after that symbol
            if(args[i + 1] != NULL)
            {
                //copy filename into struct and remove filename as an argument
                strcpy(redirStatus.outFile, args[i + 1]);
                args[i + 1] = NULL;
                i++;
            }
            else
            {
                fflush(stdout);
                printf("No filename provided after re");
            }
        }
    }

    return redirStatus;
}

int main()
{
    int exitStatus = 0;
    while(1)
    {
        fflush(stdout);
        prompt(&exitStatus);
    }
}
