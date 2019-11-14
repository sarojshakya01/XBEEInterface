/*
 * Xbee_RX_interrupt.c
 *
 * Created: 7/4/2015 10:37:41 PM
 *  Author: User
 */ 


#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>

#define LCD_DATA_PORT PORTA
#define LCD_DATA_DDR DDRA
#define LCD_RS 2
#define LCD_EN 3

#define TRUE 1
#define FALSE 0

unsigned char received_char;
int received_data_counter=0;
unsigned char received_data[20];
unsigned char received;
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
void LCD_num(uint16_t num)
{
	LCD_data(num/100+0x30);
	num = num%100;
	LCD_data(num/10 + 0x30);
	LCD_data(num%10 + 0x30);
}
void usart_initialize()//USART initialization//
{
	UCSRA = 0;
	UCSRB = (1<<TXEN) | (1<<RXEN) | (1 << RXCIE);// | (1<< UDRIE);//enable tx and rx pin and rx interupt
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
	_delay_ms(10);
	usart_send_char(number%10 + 0x30);
}
void show_received_data(int j)
{
	int i;
	for (i=0;i<j;i++)
	{
		LCD_data(received_data[i]);
	}
}
void empty_array(int j)
{
	int i;
	for (i=0;i<j;i++)
	{
		received_data[i]=' ';
	}
}
unsigned char humidity_value;
unsigned char temperature_value;
unsigned char battery_level;

void read_battery_status()
{
	ADCSRA = 0x87; //Enable ADC and select clk/128
	ADMUX = 0xE0; //0b1110000, 11 for Vref=2.56, 1 for left justified,
	ADCSRA |= 1<<ADSC; //Start conversion in ADC
	while ((ADCSRA & (1<<ADIF)) == 0); //till the end of conversion
	battery_level = ADCH; //save the converted output
	//ADCSRA = 0x00;
	LCD_num(battery_level);
}

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
	unsigned char i;
	LCD_initialize();
	LCD_cmnd(0x0C);
	LCD_print("This is RECEIVER");
	//_delay_ms(1000);
	usart_initialize();
	_delay_ms(10);
	//while(1)
	{
		LCD_clear();
		read_battery_status();
		_delay_ms(500);
	}
	sei();	
	while(1)
	{
		LCD_clear();
		if (!(received_data_counter <4))
		{
			LCD_clear();
			LCD_print("Data Received!!!");
			LCD_set_curser(2,1);
			for (i=0;i<received_data_counter;i++)
			{
				LCD_data(received_data[i]);
			}
			//usart_send_string("OK");
			if (received_data[0] == 104 && received_data[1] == 105)
			{
				read_temperature();
				//read_humidity();
				humidity_value = 65;
				_delay_ms(1000);
				usart_send_num(temperature_value);
				_delay_ms(10);
				usart_send_char(' ');
				usart_send_num(humidity_value);
				_delay_ms(10);
				usart_send_char('\n');
				usart_send_char('\r');
			}
			_delay_ms(1000);
			empty_array(received_data_counter);
			received_data_counter=0;
			sei(); //Reenable interrupt
		}
	}
}


ISR(USART_RXC_vect)
{
	received_char = UDR;
	if(received_data_counter < 4)
	{
		received_data[received_data_counter]=received_char;
		received_data_counter++;
	}
	else
	{
		//received_data_counter = -1;
		cli(); //disable interrupt
	}
	
}