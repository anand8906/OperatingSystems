#include<stdio.h>
#include<stdlib.h>

int main(int argc, char **argv)
{
    for(int i = 1; i < argc; i++ )
    {
        FILE *fp = fopen(argv[i], "r");
        if(fp == NULL)
        {
            printf("my-cat: cannot open file\n");
            exit(1);
        }
        char buffer[1024];
        while(fgets(buffer, 1024, fp) != NULL)
            printf("%s", buffer);
    }
    return 0;
}
