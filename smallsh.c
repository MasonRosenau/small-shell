#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    char* input = NULL;
    size_t len = 0;
    ssize_t read;

    while(1){
        
        printf(": ");
        fflush(stdout);
        fflush(stdin);
        //write to input, len is length of user input (including \n), and read is dynamic buffer size
        read = getline(&input, &len, stdin);
        printf("%s", input);
        printf("len: %d\n", read);
        printf("read: %d\n", len);
    
    }

    free(input);
    return 0;
}
