#include<stdio.h>
#include<string.h>
#include<stdlib.h>

int main(int argc, char **argv)
{
    if(argc == 1)
    {
        printf("my-grep: searchterm [file ...]\n");    
        exit(1);
    }
    else if (argc == 2)
    {
        char * grep_word = argv[1];
        char *line = NULL;
        size_t linecap = 0;
        ssize_t linelen;
        while((linelen = getline(&line, &linecap, stdin)) > 0)
        {
            if(strstr(line, grep_word))
                printf("%s", line);
        }
    }
    else
    {
        char * grep_word = argv[1];
        for(int i = 2; i < argc; i++ )
        {
            FILE *fp = fopen(argv[i], "r");
            if(fp == NULL)
            {
                printf("my-grep: cannot open file\n");
                exit(1);
            }
            char *line = NULL;
            size_t linecap = 0;
            ssize_t linelen;
            while((linelen = getline(&line, &linecap, fp)) > 0)
            {
                if(strstr(line, grep_word))
                    printf("%s", line);
            }
        }
    }
    return 0;
}
