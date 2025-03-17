#include <stdio.h>
#include "fancy-hello-world.h"
#include <string.h>

int main(void){
    
    char name[50];
    char output[100];
    printf("Enter name:\n"); 
    fgets(name, 50, stdin);
    hello_string(name, output);
    return 0;
}

void hello_string(char* name,char*  output) {
    //to remove \n
    char hello[80];
    snprintf(hello,79, " Hello World, hello %s",name);
    
    strncpy(output,hello, strlen(hello));
    printf("%s", output);
}
