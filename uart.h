#include<lpc21xx.h>

void uart_init()
{
	PINSEL0|=0X5;	
	U0LCR=0X83;
	U0DLL=97;
	U0DLM=0;
	U0LCR=0X03;
}	
void uart_data(unsigned char data)
{
	while(((U0LSR>>5)&1)==0);
	U0THR=data;
}	
void uart_str(unsigned char *str)
{
	while(*str)
			uart_data(*str++);
}	
