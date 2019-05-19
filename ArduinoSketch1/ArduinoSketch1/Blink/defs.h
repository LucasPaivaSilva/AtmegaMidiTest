/*
 * IncFile1.h
 *
 * Created: 5/15/2019 11:13:03 AM
 *  Author: Lucas
 */ 


#ifndef INCFILE1_H_
#define INCFILE1_H_

#define F_CPU 16000000UL	//define a frequencia do microcontrolador - 16MHz

#include <avr/io.h> 	    //defini��es do componente especificado
#include <util/delay.h>		//biblioteca para o uso das rotinas de _delay_ms e _delay_us()
#include <avr/pgmspace.h>   //para o uso do PROGMEM, grava��o de dados na mem�ria flash
#include <avr/interrupt.h>
#include <Arduino.h>
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();
//Defini��es de macros para o trabalho com bits

#define	set_bit(y,bit)	(y|=(1<<bit))	//coloca em 1 o bit x da vari�vel Y
#define	clr_bit(y,bit)	(y&=~(1<<bit))	//coloca em 0 o bit x da vari�vel Y
#define cpl_bit(y,bit) 	(y^=(1<<bit))	//troca o estado l�gico do bit x da vari�vel Y
#define tst_bit(y,bit) 	(y&(1<<bit))	//retorna 0 ou 1 conforme leitura do bit



#endif /* INCFILE1_H_ */

#ifndef _LCD_H
#define _LCD_H

//Defini��es para facilitar a troca dos pinos do hardware e facilitar a re-programa��o

#define DADOS_LCD    	PORTC  	//4 bits de dados do LCD no PORTD
#define nibble_dados	0		//0 para via de dados do LCD nos 4 LSBs do PORT empregado (Px0-D4, Px1-D5, Px2-D6, Px3-D7)
//1 para via de dados do LCD nos 4 MSBs do PORT empregado (Px4-D4, Px5-D5, Px6-D6, Px7-D7)
#define CONTR_LCD 		PORTC  	//PORT com os pinos de controle do LCD (pino R/W em 0).
#define E    			PC4     //pino de habilita��o do LCD (enable)
#define RS   			PC5     //pino para informar se o dado � uma instru��o ou caractere

#define tam_vetor	5	//n�mero de digitos individuais para a convers�o por ident_num()
#define conv_ascii	48	//48 se ident_num() deve retornar um n�mero no formato ASCII (0 para formato normal)

//sinal de habilita��o para o LCD
#define pulso_enable() 	_delay_us(1); set_bit(CONTR_LCD,E); _delay_us(1); clr_bit(CONTR_LCD,E); _delay_us(45)

//prot�tipo das fun��es
void cmd_LCD(unsigned char c, char cd);
void inic_LCD_4bits();
void escreve_LCD(char *c);
void escreve_LCD_Flash(const char *c);

void ident_num(unsigned int valor, unsigned char *disp);

void cmd_LCD(unsigned char c, char cd)				//c � o dado  e cd indica se � instru��o ou caractere
{
	if(cd==0)
		clr_bit(CONTR_LCD,RS);
	else
		set_bit(CONTR_LCD,RS);

	//primeiro nibble de dados - 4 MSB
	#if (nibble_dados)								//compila c�digo para os pinos de dados do LCD nos 4 MSB do PORT
		DADOS_LCD = (DADOS_LCD & 0x0F)|(0xF0 & c);		
	#else											//compila c�digo para os pinos de dados do LCD nos 4 LSB do PORT
		DADOS_LCD = (DADOS_LCD & 0xF0)|(c>>4);	
	#endif
	
	pulso_enable();

	//segundo nibble de dados - 4 LSB
	#if (nibble_dados)								//compila c�digo para os pinos de dados do LCD nos 4 MSB do PORT
		DADOS_LCD = (DADOS_LCD & 0x0F) | (0xF0 & (c<<4));		
	#else											//compila c�digo para os pinos de dados do LCD nos 4 LSB do PORT
		DADOS_LCD = (DADOS_LCD & 0xF0) | (0x0F & c);
	#endif
	
	pulso_enable();
	
	if((cd==0) && (c<4))				//se for instru��o de retorno ou limpeza espera LCD estar pronto
		_delay_ms(2);
}
//---------------------------------------------------------------------------------------------
//Sub-rotina para inicializa��o do LCD com via de dados de 4 bits
//---------------------------------------------------------------------------------------------
void inic_LCD_4bits()		//sequ�ncia ditada pelo fabricando do circuito integrado HD44780
{							//o LCD ser� s� escrito. Ent�o, R/W � sempre zero.

	clr_bit(CONTR_LCD,RS);	//RS em zero indicando que o dado para o LCD ser� uma instru��o	
	clr_bit(CONTR_LCD,E);	//pino de habilita��o em zero
	
	_delay_ms(20);	 		//tempo para estabilizar a tens�o do LCD, ap�s VCC ultrapassar 4.5 V (na pr�tica pode
							//ser maior). 
	//interface de 8 bits						
	#if (nibble_dados)
		DADOS_LCD = (DADOS_LCD & 0x0F) | 0x30;		
	#else		
		DADOS_LCD = (DADOS_LCD & 0xF0) | 0x03;		
	#endif						
							
	pulso_enable();			//habilita��o respeitando os tempos de resposta do LCD
	_delay_ms(5);		
	pulso_enable();
	_delay_us(200);
	pulso_enable();	/*at� aqui ainda � uma interface de 8 bits.
					Muitos programadores desprezam os comandos acima, respeitando apenas o tempo de
					estabiliza��o da tens�o (geralmente funciona). Se o LCD n�o for inicializado primeiro no 
					modo de 8 bits, haver� problemas se o microcontrolador for inicializado e o display j� o tiver sido.*/
	
	//interface de 4 bits, deve ser enviado duas vezes (a outra est� abaixo)
	#if (nibble_dados) 
		DADOS_LCD = (DADOS_LCD & 0x0F) | 0x20;		
	#else		
		DADOS_LCD = (DADOS_LCD & 0xF0) | 0x02;
	#endif
	
	pulso_enable();		
   	cmd_LCD(0x28,0); 		//interface de 4 bits 2 linhas (aqui se habilita as 2 linhas) 
							//s�o enviados os 2 nibbles (0x2 e 0x8)
   	cmd_LCD(0x08,0);		//desliga o display
   	cmd_LCD(0x01,0);		//limpa todo o display
   	cmd_LCD(0x0C,0);		//mensagem aparente cursor inativo n�o piscando   
   	cmd_LCD(0x80,0);		//inicializa cursor na primeira posi��o a esquerda - 1a linha
}
//---------------------------------------------------------------------------------------------
//Sub-rotina de escrita no LCD -  dados armazenados na RAM
//---------------------------------------------------------------------------------------------
void escreve_LCD(char *c)
{
   for (; *c!=0;c++) cmd_LCD(*c,1);
}
//---------------------------------------------------------------------------------------------
//Sub-rotina de escrita no LCD - dados armazenados na FLASH
//---------------------------------------------------------------------------------------------
void escreve_LCD_Flash(const char *c)
{
   for (;pgm_read_byte(&(*c))!=0;c++) cmd_LCD(pgm_read_byte(&(*c)),1);
}
//---------------------------------------------------------------------------------------------
//Convers�o de um n�mero em seus digitos individuais
//---------------------------------------------------------------------------------------------
void ident_num(unsigned int valor, unsigned char *disp)
{   
 	unsigned char n;

	for(n=0; n<tam_vetor; n++)
		disp[n] = 0 + conv_ascii;		//limpa vetor para armazenagem do digitos 

	do
	{
       *disp = (valor%10) + conv_ascii;	//pega o resto da divisao por 10 
	   valor /=10;						//pega o inteiro da divis�o por 10
	   disp++;

	}while (valor!=0);
}
//---------------------------------------------------------------------------------------------

#endif

