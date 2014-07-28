#include <msp430.h> 
#include <msp430f5529.h>

/*
 * main.c
 */

typedef enum {START,FIRST,SECOND} Seg;
Seg tranSeg = START;

inline unsigned char is_valid(unsigned char data);


int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    P6DIR = BIT4 + BIT3 + BIT2 + BIT1 + BIT0;

    UCA1CTL1 |= UCSWRST;
    UCA1CTL1 |= UCSSEL_2;//SMCLK
    UCA1BR0 = 109;
    UCA1BR1 = 0x00;
    UCA1MCTL |= UCBRS_2;
    P4SEL |= BIT4+BIT5;
    UCA1CTL1 &= ~UCSWRST;
    UCA1IE |= UCRXIE;
    __bis_SR_register(GIE);
    while(1);
}

#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void){
	unsigned short data = UCA1RXBUF;
	UCA1TXBUF = data;
	unsigned short LEDout = 0;
	if(data == 's'){
		LEDout = BIT0 + BIT1 + BIT2 + BIT3 + BIT4;
		tranSeg = START;//Indicate just transmitted start condition
	} else if(is_valid(data)){
		if((0x30 <= data) && (data <= 0x39)){
			data = data - 0x30;
			LEDout = ((data << 1) & 0x1c) | (data & 0x3);
		} else if((0x61 <= data) && (data <= 0x66)){
			data = data - 0x57;
			LEDout = ((data << 1) & 0x1c) | (data & 0x3);
		}
		if(tranSeg == START){
			LEDout &= ~BIT2;
			tranSeg = FIRST;
		} else if(tranSeg == FIRST){
			LEDout |= BIT2;
			tranSeg = SECOND;
		} else if(tranSeg == SECOND){
			LEDout = 0;
		}
	}
	P6OUT = LEDout;
}

inline unsigned char is_valid(unsigned char data){
	if(((0x30 <= data) && (data <= 0x39)) || ((0x61 <= data) && (data <= 0x66)))
		return 1;
	return 0;
}
