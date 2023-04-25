#include "o3.h"
#include "gpio.h"
#include "systick.h"

/**************************************************************************//**
 * @brief Konverterer nummer til string 
 * Konverterer et nummer mellom 0 og 99 til string
 *****************************************************************************/
void int_to_string(char *timestamp, unsigned int offset, int i) {
    if (i > 99) {
        timestamp[offset]   = '9';
        timestamp[offset+1] = '9';
        return;
    }

    while (i > 0) {
	    if (i >= 10) {
		    i -= 10;
		    timestamp[offset]++;
		
	    } else {
		    timestamp[offset+1] = '0' + i;
		    i=0;
	    }
    }
}

/**************************************************************************//**
 * @brief Konverterer 3 tall til en timestamp-string
 * timestamp-argumentet må være et array med plass til (minst) 7 elementer.
 * Det kan deklareres i funksjonen som kaller som "char timestamp[7];"
 * Kallet blir dermed:
 * char timestamp[7];
 * time_to_string(timestamp, h, m, s);
 *****************************************************************************/
void time_to_string(char *timestamp, int h, int m, int s) {
    timestamp[0] = '0';
    timestamp[1] = '0';
    timestamp[2] = '0';
    timestamp[3] = '0';
    timestamp[4] = '0';
    timestamp[5] = '0';
    timestamp[6] = '\0';

    int_to_string(timestamp, 0, h);
    int_to_string(timestamp, 2, m);
    int_to_string(timestamp, 4, s);
}

#define LED_PIN 2
#define LED_PORT GPIO_PORT_E
#define PB0_PIN 9
#define PB1_PIN 10
#define PB_PORT GPIO_PORT_B

volatile uint16_t seconds = 0;
volatile uint16_t minutes = 0;
volatile uint16_t hours = 0;
volatile uint8_t state = 0;

typedef struct {
	volatile word CTRL;
	volatile word MODEL;
	volatile word MODEH;
	volatile word DOUT;
	volatile word DOUTSET;
	volatile word DOUTCLR;
	volatile word DOUTTGL;
	volatile word DIN;
	volatile word PINLOCKN;
} gpio_port_map_t;

typedef struct {
	volatile gpio_port_map_t ports[6];
	volatile word unused_space[10];
	volatile word EXTIPSELL;
	volatile word EXTIPSELH;
	volatile word EXTIRISE;
	volatile word EXTIFALL;
	volatile word IEN;
	volatile word IF;
	volatile word IFS;
	volatile word IFC;
	volatile word ROUTE;
	volatile word INSENSE;
	volatile word LOCK;
	volatile word CTRL;
	volatile word CMD;
	volatile word EM4WUEN;
	volatile word EM4WUPOL;
	volatile word EM4WUCAUSE;
} gpio_map_t;

volatile gpio_map_t* GPIOpointer;

typedef struct {
	volatile word CTRL;
	volatile word LOAD;
	volatile word VAL;
	volatile word CALIB;
} systick_map_t;

volatile systick_map_t* systickPointer;

void updateDisplay(void){
    char timeString[7];
	time_to_string(timeString, hours, minutes, seconds);
	lcd_write(timeString);
}

//Interrupt vector for PB0
void GPIO_ODD_IRQHandler(void) {
	if (state == 0){
		seconds ++;
		if (seconds == 60){
			seconds = 0;
		}
	}
	else if (state == 1){
		minutes ++;
		if (minutes == 60){
			minutes = 0;
		}
	}
	else if (state == 2){
		hours ++;
		if (hours == 100){
			hours = 0;
		}
	}

	updateDisplay();

	//Clear IF register after interrupt is handled
	GPIOpointer -> IFC = 1 << PB0_PIN;
}

//Interrupt vector for PB1
void GPIO_EVEN_IRQHandler(void) {
	//Go to edit minutes state
	if (state == 0){
		state++;
	}
	//Go to edit hours state
	else if (state == 1){
		state++;
	}
	//Go to countdown state
	else if (state == 2){
		//Enable systick to start countdown
		state++;
		if(seconds != 0){
			seconds--;
		}
		updateDisplay();
		systickPointer -> CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
	}
	//Go to edit seconds state
	else if (state == 4){
		GPIOpointer -> ports[LED_PORT].DOUTCLR = 1 << LED_PIN;
		state = 0;
	}

	//Clear IF register after interrupt is handled
	GPIOpointer -> IFC = 1 << PB1_PIN;
}

void SysTick_Handler(void) {
	if (seconds == 0){
		if (minutes == 0) {
			if (hours == 0){
				//Countdown finished
				systickPointer -> CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
				GPIOpointer -> ports[LED_PORT].DOUTSET = 1 << LED_PIN;
				state++;
			}
			else {
				seconds = 59;
				minutes = 59;
				hours--;
			}
		}
		else {
			seconds = 59;
			minutes--;
		}
	}
	else{
		seconds--;
	}

	updateDisplay();
}

int main(void) {
    init();
    
    GPIOpointer = (gpio_map_t*) GPIO_BASE;

    //Set up LED (port E, pin 2) as output
    GPIOpointer -> ports[LED_PORT].DOUT = 0;
    GPIOpointer -> ports[LED_PORT].MODEL = GPIOpointer -> ports[LED_PORT].MODEL | (GPIO_MODE_OUTPUT << LED_PIN*4);

    //Set up PB0 (port B, pin 9) and PB1 (port B, pin 10) as input
    GPIOpointer -> ports[PB_PORT].MODEH = GPIOpointer -> ports[PB_PORT].MODEH | (GPIO_MODE_INPUT << 4);
    GPIOpointer -> ports[PB_PORT].MODEH = GPIOpointer -> ports[PB_PORT].MODEH | (GPIO_MODE_INPUT << 8);

    //Set up EXTIPSELH
	GPIOpointer -> EXTIPSELH = (~(0b1111 << 4) & GPIOpointer -> EXTIPSELH) | (1 << 4);
	GPIOpointer -> EXTIPSELH = (~(0b1111 << 8) & GPIOpointer -> EXTIPSELH) | (1 << 8);

	//Set up EXTIFALL
	GPIOpointer -> EXTIFALL = GPIOpointer -> EXTIFALL | (1 << PB0_PIN);
	GPIOpointer -> EXTIFALL = GPIOpointer -> EXTIFALL | (1 << PB1_PIN);

	//Set up IEN
	GPIOpointer -> IEN = GPIOpointer -> IEN | (1 << PB0_PIN);
	GPIOpointer -> IEN = GPIOpointer -> IEN | (1 << PB1_PIN);

    systickPointer = (systick_map_t*) SYSTICK_BASE;

    //Set up CTRL register, but not enabling systick yet
    systickPointer -> CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;

    //Set frequency to 14000000 = 1 second
    systickPointer -> LOAD = FREQUENCY;

    updateDisplay();

    while(1){

    }

    return 0;
}

