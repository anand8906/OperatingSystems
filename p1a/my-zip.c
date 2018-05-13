#include<stdio.h>
#include<stdlib.h>

int main(int argc, char **argv)
{
    if(argc == 1)
    {
        printf("my-zip: file1 [file2 ...]\n");
        exit(1);
    }
    char prev;
    char curr;
    int count = 0;
    int first = 0;
    for(int i = 1; i < argc; i++ )
    {
        FILE *fp = fopen(argv[i], "r");
        if(fp == NULL)
        {
            printf("my-zip: cannot open file\n");
            exit(1);
        }
        char *line = NULL;
        size_t linecap = 0;
        ssize_t linelen;
        while((linelen = getline(&line, &linecap, fp)) > 0)
        {

            for(int i = 0; line[i] != '\0'; i++)
            {
                if(first == 0) {
                    prev = line[i];
                    count++;
                    first = 1;
                    continue;
                }
                curr = line[i];
                if(prev != curr) {
                    fwrite(&count, sizeof(int), 1, stdout);
                    fwrite(&prev, sizeof(char), 1, stdout);
                    prev = curr;
                    count = 0;
                }
                count++;
            } 
        }
        free(line);
        fclose(fp);
    }
    fwrite(&count, sizeof(int), 1, stdout);
    fwrite(&prev, sizeof(char), 1, stdout);
    return 0;
}

