
#include "timer.h"


#define RS 1<<2
#define EN 1<<3
#define led_d 0x0f<<4

void init(void);
void lcd_cmd(unsigned char);
void data(unsigned char);
void lcd_str(unsigned char*);
void enable(void);


void enable()
{
	IOSET0=EN;
	delay_ms(2);
	IOCLR0=EN;
}												
				
void init()
{
	IODIR0|=led_d|RS|EN;
	lcd_cmd(0x01);
	lcd_cmd(0x02);
	lcd_cmd(0x0c);
	lcd_cmd(0x28);
	lcd_cmd(0x80);
}
void lcd_cmd(unsigned char cmd)	
{	
	IOCLR0=RS;//MSB
	IOSET0=(((cmd&0xf0)>>4)<<4);
	enable();
	IOCLR0=led_d;
	
	
	IOCLR0=RS;//LSB
	IOSET0=((cmd&0x0f)<<4);
	enable();
	IOCLR0=led_d;
}
void data(unsigned char data)
{	
	IOSET0=RS; //MSB
	IOSET0=(((data&0xf0)>>4)<<4);
	enable();
	IOCLR0=led_d;
	
	IOSET0=RS; //LSB
	IOSET0=((data&0x0f)<<4);
	enable();
	IOCLR0=led_d;
}
void lcd_str(unsigned char *str)
{	
	while(*str)
	{
		data(*str++);
	}
}
