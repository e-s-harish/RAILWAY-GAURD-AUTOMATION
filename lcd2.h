#include<lpc21xx.h>
#define rs 1<<24
#define en 1<<25
#define lcd 0x0f<<27
#define sf <<27

void init2(void);
void lcd_cmd2(unsigned char);
void data2(unsigned char);
void lcd_str2(unsigned char*);
void enable2(void);
void delay_ms(unsigned char );
void delay_ms(unsigned char);


void enable2()
{
	IOSET0=en;
	delay_ms(2);
	IOCLR0=en;
}												
				
void init2()
{
	//PINSEL1 |= 0<<27;
	PINSEL1 &= ~(0xff<<22);
	IODIR0|=lcd|rs|en;
	lcd_cmd2(0x01);
	lcd_cmd2(0x02);
	lcd_cmd2(0x0c);
	lcd_cmd2(0x28);
	lcd_cmd2(0x80);
}
void lcd_cmd2(unsigned char cmd)	
{	
	IOCLR0=rs;//MSB
	IOSET0=(((cmd&0xf0)>>4)sf);
	enable2();
	IOCLR0=lcd;
	
	
	IOCLR0=rs;//LSB
	IOSET0=((cmd&0x0f)sf);
	enable2();
	IOCLR0=lcd;
}
void data2(unsigned char data)
{	
	IOSET0=rs; //MSB
	IOSET0=(((data&0xf0)>>4)sf);
	enable2();
	IOCLR0=lcd;
	
	IOSET0=rs; //LSB
	IOSET0=((data&0x0f)sf);
	enable2();
	IOCLR0=lcd;
}
void lcd_str2(unsigned char *str)
{	
	while(*str)
	{
		data2(*str++);
	}
}

