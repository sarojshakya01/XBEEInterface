/*
 * Xbee_Transmitter.c
 *
 * Created: 6/25/2015 5:32:40 PM
 *  Author: Er. Saroj Shakya
 */ 
 
#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>

//---LCD Port Definition---//
#define LCD_DATA_PORT PORTA
#define LCD_DATA_DDR DDRA
#define LCD_RS 2
#define LCD_EN 3
//-------------------------//
#define delay_tx 10

unsigned char received_data[200];

//---LCD Code starts from here---//
void LCD_cmnd(unsigned char cmnd)
{
	LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (cmnd & 0xF0); //send upper 4 bit
	LCD_DATA_PORT &= ~(1<<LCD_RS); //0b11111011 //RS = 0
	LCD_DATA_PORT |= 1<<LCD_EN; //0b00001000 //EN = 1
	_delay_us(50);
	LCD_DATA_PORT &= ~(1<<LCD_EN); //0b11110111 //EN = 0
	_delay_us(200);
	LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (cmnd << 4); //send lower 4 bit
	LCD_DATA_PORT |= 1<<LCD_EN; //0b00001000 //EN = 1
	_delay_us(50);
	LCD_DATA_PORT &= ~(1<<LCD_EN); //0b11110111 //EN = 0
}
void LCD_data(unsigned char data)
{
	LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (data & 0xF0);
	LCD_DATA_PORT |= 1<<LCD_RS; //0b00000100 //RS = 1
	LCD_DATA_PORT |= 1<<LCD_EN; //0b00001000
	_delay_us(50);
	LCD_DATA_PORT &= ~(1<<LCD_EN); //0b11110111
	_delay_us(2000);
	LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (data << 4);
	LCD_DATA_PORT |= 1<<LCD_EN; //0b00001000
	_delay_us(2000);
	LCD_DATA_PORT &= ~(1<<LCD_EN); //0b11110111
}
void LCD_initialize(void)
{
	LCD_DATA_DDR = 0xFC;
	LCD_DATA_PORT &= ~(1<<LCD_EN); //0b11110111;
	_delay_ms(200);
	LCD_cmnd(0x33);
	_delay_ms(20);
	LCD_cmnd(0x32);
	_delay_ms(20);
	LCD_cmnd(0x28);
	_delay_ms(20);
	LCD_cmnd(0x0E);
	_delay_ms(20);
	LCD_cmnd(0x01);
	_delay_ms(20);
}
void LCD_clear(void)
{
	LCD_cmnd(0x01);
	_delay_ms(2);
}
void LCD_print(char * str)
{
	unsigned char i=0;
	while(str[i] != 0)
	{
		LCD_data(str[i]);
		i++;
		_delay_ms(5);
	}
}
void LCD_set_curser(unsigned char y, unsigned char x)
{
	if(y==1)
	LCD_cmnd(0x7F+x);
	else if(y==2)
	LCD_cmnd(0xBF+x);
}
void LCD_num(unsigned char num)
{
	//LCD_data(num/100 + 0x30);
	//num = num%100;
	LCD_data(num/10 + 0x30);
	LCD_data(num%10 + 0x30);
}
//---LCD Code ends here---//
void usart_initialize()//USART initialization//
{
	UCSRB = (1<<TXEN) | (1<<RXEN) | (1<<RXCIE); //enable tx and rx pin
	UCSRC = (1<<UCSZ0) | (1<<UCSZ1) | (1<<URSEL); //character size 8, 1 stop bit and reg select bit = 1
	UBRRL = 0x67; //baud rate 9600
}
void usart_send_char(unsigned char txdata)//Function to send single character serially//
{
	while(!(UCSRA&(1<<UDRE)));
	UDR = txdata;
}
void usart_send_string(char *str)//Function to send string serially//
{
	unsigned char i=0;
	while(str[i] != 0)
	{
		usart_send_char(str[i]);
		i++;
		_delay_ms(5);
	}
}
void usart_send_num(unsigned char number)
{
	usart_send_char(number/10 + 0x30);
	_delay_ms(delay_tx);
	usart_send_char(number%10 + 0x30);
}
unsigned char usart_receive_char()
{
	while( !(UCSRA & (1<<RXC)) );
	return UDR;
}
void usart_receive_string()
{
	unsigned char i=0;
	while(i != 90)
	{
		received_data[i] = usart_receive_char();
		i++;
	}
}

//-------------------------//

unsigned char humidity_value;
unsigned char temperature_value;

//---Temperature reading function---//
void read_temperature()
{
	ADCSRA = 0x87; //Enable ADC and select clk/128
	ADMUX = 0xE0; //0b1110000, 11 for Vref=2.56, 1 for left justified,
	ADCSRA |= 1<<ADSC; //Start conversion in ADC
	while ((ADCSRA & (1<<ADIF)) == 0); //till the end of conversion
	temperature_value = ADCH; //save the converted output
	ADCSRA = 0x00;
}
//-------------------------//

/*void read_humidity()//Function to read temperature
{
	ADCSRA = 0x87; //Enable ADC and select clk/128
	ADMUX = 0xE1; //0b1110001, 11 for Vref=2.56, 1 for left justified,ADC1
	ADCSRA |= 1<<ADSC; //Start conversion in ADC
	while ((ADCSRA & (1<<ADIF)) == 0); //till the end of conversion
	humidity_value = ADCH; //save the converted output
	ADCSRA = 0;
}*/
//-------------------------//

int main(void)
{
	LCD_initialize();
	LCD_cmnd(0x0C);
	LCD_print("TRANSMITTER");
	_delay_ms(2000);
	usart_initialize();
	_delay_ms(20);
	//usart_send_string("+++");
	_delay_ms(20);
	while(1)
	{
		read_temperature();
		//read_humidity();
		//LCD_clear();
		//LCD_num(temperature_value);
		//LCD_num(humidity_value);
		_delay_ms(1000);
		usart_send_num(temperature_value);
		_delay_ms(delay_tx);
		usart_send_char(0xDF);
		_delay_ms(delay_tx);
		usart_send_char('C');
		//_delay_ms(delay_tx);
		//usart_send_num(humidity_value);
		//_delay_ms(10);
		//usart_send_char('%');
		//_delay_ms(1000);
	}
}

