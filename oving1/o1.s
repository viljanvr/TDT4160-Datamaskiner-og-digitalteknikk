.thumb
.syntax unified

.include "gpio_constants.s"     // Register-adresser og konstanter for GPIO

.text
	.global Start



Start:
    LDR R0, =GPIO_BASE
    LDR R1, =PORT_SIZE

    LDR R2, =LED_PORT
    MUL R2, R2, R1
    ADD R2, R2, R0

    LDR R3, =GPIO_PORT_DOUTSET
    ADD R3, R3, R2 //Set address of DOUTSET register inn R3
    LDR R4, =GPIO_PORT_DOUTCLR
    ADD R4, R4, R2 //Set address of DOUTCLR register inn R4

    LDR R2, =BUTTON_PORT
    MUL R2, R2, R1
    ADD R2, R2, R0
    LDR R5, =GPIO_PORT_DIN
    ADD R2, R2, R5 // Set address of DIN register inn R2


	MOV R5, #1
	LSL R5, R5, #LED_PIN //Create value to turn light on and off

	MOV R6, #1
	LSL R6, R6, #BUTTON_PIN //Create mask

	B Off

Loop:
	LDR R7, [R2] //Load DIN
	AND R7, R7, R6
	CMP R7, R6
	BEQ Off //When the bit for the button pin in DIN is high, turn the light off

On:
	STR R5, [R3]
	B Loop

Off:
	STR R5, [R4]
	B Loop


NOP // Behold denne på bunnen av fila

