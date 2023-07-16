#include<stdio.h>
#include<stdlib.h>
int main()
{
    int i;
    int *ptr = (int*)malloc(sizeof(int));

    if(ptr==NULL)
    {
        printf("Memory allocation failed\n");
        return 1;
    }

     *ptr = 25;

    printf("Value: %d\n", *ptr);

    free(ptr);
    
    return 0;
}