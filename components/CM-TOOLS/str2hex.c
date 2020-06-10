#include "str2hex.h"
#include	<stdio.h>
#include	<stdlib.h>
#include <string.h>


/**
 * @brief
 *
 * @param c
 *
 * @return
 */
static unsigned char str2hexnum(unsigned char c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if(c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0; /* foo */
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  str2hex
 *  Description:
 * =====================================================================================
 */

static char str2hex( char *str)
{
	int value = 0;
	while (*str) {
		value = value << 4;
		value |= str2hexnum(*str++);
	}

	return value;

    return value;
}		/* -----  end of function str2hex  ----- */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:
 * =====================================================================================
 */
char* test (  const char *src)
{
    // char *str= "2324";


    char xx[3] = {0};
    char *tmp = (char*)malloc(51*sizeof(char));


    size_t size = strlen(src)/2;

    for(int i =0; i < size; i++){
      memcpy(xx, src, 2);

      tmp[i] = str2hex(xx);
      src++;
      src++;
   }
   tmp[size] = '\0';
    // printf("\n");
    //
    //   for(int i =0; i < size; i++)
    //         printf("[%02x] ", tmp[i]);

    return tmp;
}				/* ----------  end of function main  ---------- */


int array_to_num(int arr[],int n){
    char str[6][3];
    int i;
    char number[13] = {'\n'};

    for(i=0;i<n;i++) sprintf(str[i],"%d",arr[i]);
    for(i=0;i<n;i++)strcat(number,str[i]);

    i = atoi(number);
    return i;
}
