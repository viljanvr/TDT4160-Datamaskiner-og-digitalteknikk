.thumb
.syntax unified

.include "gpio_constants.s"     // Register-adresser og konstanter for GPIO
.include "sys-tick_constants.s" // Register-adresser og konstanter for SysTick

.text
	.global Start

Start:

	//Set up Systick
	LDR R0, =SYSTICK_BASE

	//Set up systick CTRL
	LDR R1, =SYSTICK_CTRL
	ADD R1, R1, R0
	MOV R2, #0b110
	STR R2, [R1]

	//Set up systick LOAD
	LDR R1, =SYSTICK_LOAD
	ADD R1, R1, R0
	LDR R2, =FREQUENCY/10
	STR R2, [R1]

	//Set up GPIO
    LDR R0, =GPIO_BASE

	//Set up EXTIPSELH
    LDR R1, =GPIO_EXTIPSELH
    MOV R2, 0b1111
    LSL R2, R2, #4
    MVN R2, R2 //Make masks
    LDR R3, [R0, R1]
    AND R2, R2, R3 //Reset pin 9
    MOV R3, 0b0001
    LSL R3, R3, #4
    ORR R2, R2, R3 //Add 0001 to pin 9
    STR R2, [R0, R1]

    //Create button_pin mask
    MOV R4, #1
	LSL R4, R4, #BUTTON_PIN //Store mask in R4

    //Set up EXTIFALL
    LDR R1, =GPIO_EXTIFALL
    LDR R2, [R0, R1]
    ORR R2, R2, R4
    STR R2, [R0, R1]

    //Set up IEN
    LDR R1, =GPIO_IEN
    LDR R2, [R0, R1]
    ORR R2, R2, R4
    STR R2, [R0, R1]

	//Set adress of DOUTTGL register
	LDR R1, =PORT_SIZE
    LDR R2, =LED_PORT
    MUL R2, R2, R1
    ADD R2, R2, R0
    LDR R5, =GPIO_PORT_DOUTTGL
    ADD R5, R5, R2 //Store address in R5

	//Create value to write to DOUTTGL
    MOV R6, #1
	LSL R6, R6, #LED_PIN //Stores value in R6

Loop:
	B Loop

Tenths:
	LDR R0, =tenths
	LDR R1, [R0]
	ADD R1, R1, #1 //Increment tenths
	STR R1, [R0] //Update display

	CMP R1, #9 //If the number is not 9, skip
	BNE Tenths_no_overflow

	MOV R1, #0 //Reset tenths counter
	STR R1, [R0] //Update display

	PUSH {LR}
    BL Seconds
    POP {LR}

Tenths_no_overflow:
	BX LR

Seconds:
	LDR R0, =seconds
	LDR R1, [R0]
	ADD R1, R1, #1 //Increment seconds
	STR R1, [R0] //Update display

	CMP R1, #59 //If the number is not 59, skip
	BNE Seconds_no_overflow

	MOV R1, #0 //Reset seconds counter
	STR R1, [R0] //Update display

	PUSH {LR}
    BL Minutes
    POP {LR}

Seconds_no_overflow:
	STR R6, [R5] //Toggle the LED

	BX LR

Minutes:
	LDR R0, =minutes
	LDR R1, [R0]
	ADD R1, R1, #1 //Increment minutes
	STR R1, [R0] //Update display

	CMP R1, #59 //If the number is not 59, skip
	BNE Minutes_no_overflow

	MOV R1, #0 //Reset minutes counter
	STR R1, [R0] //Update display

Minutes_no_overflow:
	BX LR

Button_pressed:
	//Check if systick is enabled or not
	LDR R0, =SYSTICK_BASE
	LDR R1, =SYSTICK_CTRL
	LDR R2, [R1, R0]
	MOV R3, #0b111
	AND R2, R2, R3
	CMP R2, #0b111
	BEQ Stopwatch_pause //Pause if systick was enabled

Stopwatch_continue:
	MOV R2, #0b111
	STR R2, [R1, R0]

	BX LR
Stopwatch_pause:
	MOV R2, #0b110
	STR R2, [R1, R0]

	BX LR

.global GPIO_ODD_IRQHandler
.thumb_func
GPIO_ODD_IRQHandler:
	//Reset IF - Interrupt Flag
	LDR R0, =GPIO_BASE
    LDR R1, =GPIO_IFC
    STR R4, [R0, R1]

    PUSH {LR}
    BL Button_pressed
    POP {LR}

    BX LR

.global SysTick_Handler
.thumb_func
	SysTick_Handler:

	PUSH {LR}
	BL Tenths
	POP {LR}

	BX LR

NOP // Behold denne på bunnen av fila

