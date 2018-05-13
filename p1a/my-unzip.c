#include<stdio.h>
#include<stdlib.h>

int main(int argc, char **argv)
{
    if(argc == 1)
    {
        printf("my-unzip: file1 [file2 ...]\n");
        exit(1);
    }
    for(int i = 1; i < argc; i++ )
    {
        FILE *fp = fopen(argv[i], "rb");
        if(fp == NULL)
        {
            printf("my-unzip: cannot open file\n");
            exit(1);
        }
        int count;
        char letter;
        while(fread(&count, sizeof(int), 1, fp) != 0)
        {
            fread(&letter, sizeof(char), 1, fp);
            for(int i = 0; i < count; i++)
                printf("%c", letter);
        }
    }
    return 0;
}

