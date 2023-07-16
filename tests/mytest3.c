#include<stdio.h>
#include<stdlib.h>

int main()
{
    int i;
    for(i=0;i<5;i++)
    {
        int* ptr = (int*)malloc(sizeof(int));
    

      if(ptr==NULL)
      {
        printf("Memory allocation failed\n");
        return 1;
      }

      *ptr = i * 10;
      printf("Value of allocated memory: %d\n", *ptr);
      free(ptr);
    }
    return 0;
}