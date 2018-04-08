#include<time.h>
#include<stdio.h>
#include<string.h>

int main () {
	time_t current = time(NULL);
	printf("%s", ctime(&current));
	return 0;
}
