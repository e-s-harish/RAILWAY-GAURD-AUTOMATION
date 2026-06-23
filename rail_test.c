#include "lcd.h"
#include "lcd2.h"
#include "uart.h"
#include<stdio.h>

char msg[] = "RAILGUARD:INTELLIGENT GSM BASED AUTOMATIC RAILWAY GATE SYSTEM	";
char i=0,j;
volatile char flag = 0;
unsigned int number = 0;
unsigned char count[12];
unsigned char check=0;
unsigned char uart[25];

volatile unsigned char gate_alert = 0;
volatile static unsigned char p2 = 0;
volatile static unsigned char leave = 0;
volatile static unsigned char leave_verify=0;
volatile static unsigned char gate_open=0;


void motor(int);
void stop_m(void);
void gsm(void);

void LEAVE(void) __irq
{
	EXTINT=1<<3;
	VICSoftIntClr=(1<<17);
	if((leave_verify==0) &&(flag==3))
	{	
		T1TCR=0X03;
		T1TCR=0X00;
		T1TCR=0X01;
		leave=1;
	}
	else if(leave_verify==1)
	{
		leave=0;
		gate_open=1;
		T1TCR=0X03;
		T1TCR=0X00;
		T1TCR=0X01;
	}
	VICVectAddr=0;
}

void GATE(void) __irq
{
	EXTINT=1<<2; //ext2
	if(p2==0)
	{
	 if(flag==2)
    {
        IOSET0 |= 1<<17;
        gate_alert = 1;   
    }
    else if(flag==3)
    {
			IOCLR0 |= 1<<17;
      gate_alert = 2;
    }
		p2++;
	}
	VICVectAddr=0;
}
void IR(void) __irq
{
	VICSoftIntClr=(1<<14);
	if(flag!=2)
	{
		check=0;
		T1TCR=0X03;
		T1TCR=0X00;
		T1TCR=0X01;
		flag=1;
	}
	VICVectAddr=0;
}

void CHECK(void) __irq
{
	EXTINT=1<<1;
	if(flag!=2)
	{
		T1TCR=0X03;
		T1TCR=0X00;
		T1TCR=0X01;
		check=1;
	}
	VICVectAddr=0;
}

int main()
{
	PINSEL0 |= (unsigned int)1<<31; //EXT2
	PINSEL0 |= 1<<29; //EXT1
	PINSEL0 |= 3<<18;
	PINSEL1 |= 1<<0;//EXT0
	PINSEL1 &= ~(0xff<<22); //lcd2
	
	IODIR0 |= 1<<17; //LED P0.17 gate not close
	IODIR1|=1<<16;//MOTOR
	IODIR1|=1<<17;//gate closed
	IODIR1|=1<<20;//mm
	IODIR1|=1<<21;//MM
	
	
	IODIR1|=1<<18;
	IOSET1=1<<18;
	
	IODIR1|=1<<19;
	
	init();
	init2();
	uart_init();
	lcd_str2((unsigned char*)"Clean");
	VICIntSelect=0;
	
	VICVectCntl0=(1<<5)|15;
	VICVectAddr0=(unsigned long)CHECK;//ext1
	
	VICVectCntl1=(1<<5)|14;
	VICVectAddr1=(unsigned long)IR;//ext0
	
	VICVectCntl2=(1<<5)|16;
	VICVectAddr2=(unsigned long)GATE;//ext2
	
	VICVectCntl3=(1<<5)|17;
	VICVectAddr3=(unsigned long)LEAVE;//ext3
	
	EXTMODE |=(0X01)<<1; //EXT1 
	EXTPOLAR |=(0X00)<<1;
	
	EXTMODE |=(0X01)<<2; //EXT2
	EXTPOLAR |=(0X00)<<2;
	
	EXTMODE |=(0X01)<<3; //EXT3 
	EXTPOLAR |=(0X00)<<3;
	
	T1PR=(15000000)-1;

	VICIntEnable |=1<<17;//3
	VICIntEnable |=1<<16;//2
	VICIntEnable |=1<<15;//1
	VICIntEnable |=1<<14;//0
	
	while(1)
	{
		if(gate_open==1) //train leave ,check train leave for 5 sec then only intrrupt occurs
		{
			lcd_cmd(0x80);
			lcd_str((unsigned char *)"TRAIN LEAVED");
			
			lcd_cmd(0xC0);
			sprintf((char *)count,"GATE OPEN:%02lu",T1TC);
			lcd_str(count);
			
			if(T1TC>= 2)	//5
			{
				gsm();
				uart_str((unsigned char *)"TRAIN LEAVED\rGATE OPEN \r");
				uart_data(0x1A);
				delay_ms(50);
				
				T1TCR=0X03;
				T1TCR = 0x00;    
				IOSET1=1<<18;
				IOCLR1=1<<19;
				lcd_cmd2(0x01);
        lcd_cmd(0x01);
				
        lcd_cmd(0x80);
        lcd_str((unsigned char *)"GATE OPENED");

				//IOSET1=1<<16;//MOTOR OPEN
				motor(0); //anti clock motor
				delay_sec(5);
			
				//IOCLR1=1<<16;
				stop_m();
				IOCLR1=1<<17;//GC
				
				gate_open=0;
	      flag = 0;
				number = 0;
				check=0;
				gate_alert = 0;
				p2 = 0;
				leave = 0;
				leave_verify=0;
				gate_open=0;
			
			}
		}
		else if(leave==1)
		{
			if(T1TC>=3)//5
			{
				leave_verify=1;
				VICSoftInt=(1<<17);
			}
			else if((((IOPIN0>>8)&1)==1)&&(leave==1)) //ext1
			{
				leave=0;
				T1TCR=0X03;
				T1TCR=0X00;
			}
		}
		else if(check==1) // check train comming for 3 sec then only intrrupt occurs
		{
			if(T1TC>=2)//5
				VICSoftInt=(1<<14);
			
			if((((IOPIN0>>10)&1)==1)&&(check==1)) //ext1
			{
				check=0;
				T1TCR=0X03;
				T1TCR=0X00;
			}
		}
		else if(gate_alert == 1)
		{
			lcd_cmd2(0x80);
			
			gsm();
			uart_str((unsigned char *)"TRAIN ARRIVED CHECK POINT 2\rCLOSE GATE FAST \r");
			uart_data(0x1A);
			delay_ms(50);
			
			lcd_str2((unsigned char *)"TRAIN ARRIVED P2");
			lcd_cmd2(0xc0);
			lcd_str2((unsigned char *)"CLOSE GATE FAST");
			gate_alert = 0;
		}
		else if(gate_alert == 2)	
		{
			if(flag==3)
			{
				gsm();
				uart_str((unsigned char *)"TRAIN ARRIVED CHECK POINT 2\rGATE CLOSED \r");
				uart_data(0x1A);
				delay_ms(50);
				
				lcd_cmd2(0x01);
				lcd_cmd2(0x80);
				lcd_str2((unsigned char *)"TRAIN ARRIVED P2");
				lcd_cmd2(0xc0);
				lcd_str2((unsigned char *)"GATE CLOSED");
				gate_alert = 0;	
			}
		}
		else if(gate_alert == 3)	
		{
			lcd_cmd2(0x01);
			gate_alert = 0;	
		}
		else if(flag==0) //title scroll
		{
			for(i = 0; msg[i + 15] != '\0' && (flag==0); i++)
			{
					lcd_cmd(0x80);
					for(j = 0; j < 16 && (flag==0); j++)
					{	
							data(msg[i + j]);
					}
			}
		}
		else if(flag == 1)
		{
			
			gsm();
			uart_str((unsigned char *)"TRAIN ARRIVED CHECK POINT 1\r\n");
			uart_str((unsigned char *)"GATE CLOSE WITHIN 5 sec\r\n");
			uart_data(0x1A);
			delay_ms(50);
			
			lcd_cmd(0x01);
			IOCLR1=1<<18;
			IOSET1=1<<19;
			lcd_cmd(0x80);
			lcd_str((unsigned char *)"TRAIN ARRIVED P1");

			flag = 2;
		}
		else if(flag == 2)
		{
			lcd_cmd(0xC0);
			sprintf((char *)count,"GATE:%02lu",T1TC);
			lcd_str(count);
			//IOSET1=1<<16;
			motor(1); //clock motor
			
			if(T1TC >= 5)	//5
			{
				gsm();
				uart_str((unsigned char *)"GATE CLOSED\r");
				uart_data(0x1A);
				delay_ms(50);
				
				T1TCR=0X03;
				T1TCR = 0x00;    

        lcd_cmd(0x01);
        lcd_cmd(0x80);
        lcd_str((unsigned char *)"GATE CLOSED");

				IOCLR0=1<<17;//GNC
				//IOCLR1=1<<16;//MOTOR stop
				stop_m(); //motor stop
				IOSET1=1<<17;//GC
        flag = 3;
				
				if(p2==1)
				{
					lcd_cmd2(0x01);
					lcd_cmd2(0x80);
					lcd_str2((unsigned char *)"GATE CLOSED");
				}
			}
		}
	}
}
void motor(int f)
{
	if (f==0)//clock
	{
		IOSET1=1<<20;
		IOCLR1=1<<21;
	}	
	else//anti clock
	{
		IOSET1=1<<21;
		IOCLR1=1<<20;
	}	
}
void stop_m()
{
		IOCLR1=1<<20;
		IOCLR1=1<<21;
}	
void gsm()
{
	uart_str((unsigned char *)"AT\r\n");
	uart_str((unsigned char *)"ATEO\r\n");
  delay_ms(200);

  uart_str((unsigned char *)"AT+CMGF=1\r\n");
  delay_ms(200);

	uart_str((unsigned char *)"AT+CMGS=\"+91xxxxxxxxx\"\r\n");
	delay_ms(200);	
	
}
