#include <stdio.h>

void main1(int argc,char **argv)
{
	char *p, *last_p;
	int flag=0;
	int size = 0;
	last_p =  (char *) malloc (sizeof(char));
	printf("heap_buttom : 0x%x\n", last_p);
	while(1){
		p =  (char *) malloc (sizeof(char));

		if(p != last_p){
			size += last_p - p;
			last_p = p;
		}
		else{
			int esp;
			//__asm__ __volatile__("movl %%esp, %0;":"+m"(esp));
			printf("can't alloc more!but program still running! \n \
					esp : 0x%x   size_now : 0x%x\n \
					heap_now : 0x%x", esp, size, p);
			//return 0;
		}
	}
}