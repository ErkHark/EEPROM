#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

//ADDR Shift Register
#define SER_OUT 0
#define SFT_CLK 1
#define SRCLR 2
#define R_CLK 3

//Data Shift Registers
#define D_SER_OUT 5
#define D_SFT_CLK 6
#define D_SRCLR 7
#define D_R_CLK 8
#define D_SHF_EN 9

//ShiftIn Pins
#define PARALLEL_LOAD 10 //Active Low (!!!Jumper Wire needed!!!)
#define CLK_SFT_IN 11
#define CLK_EN 12
#define SER_IN 13

//EEPROM Pins
#define WRITE_EN 14 //Active Low
#define OUTPUT_EN 15 //Active Low
#define CHIP_SEL 16 //Active Low


//Timing
#define SFT_PER 25
#define HOLD_US 10

#define ENDSTDIN	255
#define CR		13

const uint LED_PIN = PICO_DEFAULT_LED_PIN;


int addr_shiftOut(uint16_t data);
int data_shiftOut(uint16_t data);
void pulsePin_us(int pin, int delay);
void toBin(uint16_t hexIn);
uint16_t toHex(char * stringIn);
uint16_t shiftIn();
void setDataDir(int dir);
void writeBits_16(uint16_t addr, uint16_t data);
void delay_64ns();
void delay_128ns();
void InitGPIO();

struct InputData{
	uint16_t addr;
	uint16_t data;
};

int main(){

	stdio_init_all();
	InitGPIO();

	uint16_t endAddr = 0xFFFF;
	unsigned int progress = 0;

	u_int16_t read_addr;

	setDataDir(0);
	gpio_put(CHIP_SEL, 0);

	int mem_size = 5;
	int errors = 0;
	char *strg = malloc(mem_size);
	char chr;
	char mode;
	int lp = 0;
	int msg_disp = 0;
	int done = 0;
	uint16_t addrInput = 0;
	uint16_t dataInput = 0;

	struct InputData InData;

while (true) { //main loop

	chr = getchar(); //get first char

	while(chr != ENDSTDIN){

		strg[lp++] = chr;//set the value of string[lp+1] to chr input


		if(chr == 10 || lp == (mem_size - 1)){
			strg[lp] = 0;	//terminate string
			lp = 0;		//reset string buffer pointer

			if (strg[0] == 'D'){
				/* Make a debug menu to test each function individually. This will probably
				only serve to further make the code look like spaghetti
				*/

				while(true){
					//main debug loop

					if (msg_disp == 0){
						printf("Enter A debug command: \n");
						printf("A: Address Shift Out\nD: Data Shift Out\nV: Data Shift In\n");
						printf("R: Set Data Direction to Read Mode\nW: Set Data Direction to Write Mode\n");
						printf("Q: Quit Debug Mode\n");
						msg_disp = 1;
					}

					chr = getchar();

					//should prolly replace with swtich case if I plan on expanding more
					if(chr == 'A') {
						addr_shiftOut(0xA501);
						printf("Shifted out 0xA501 to Address Registers\n");
					}

					else if(chr == 'D') {
						data_shiftOut(0xA501);
						printf("Shifted out 0xA501 to Data Registers\n");
					}

					else if(chr == 'V'){
						printf("%x\n", shiftIn(0));
					}

					else if(chr == 'R') {
						setDataDir(1);
						printf("Set Data Direction to Read\n");
					}
					else if (chr == 'W'){
						setDataDir(0);
						printf("Srt Data Direction to Write\n");
					}
					else if (chr == 'Q') {
						msg_disp = 0;
						break;
					}
			}
		}

			if (strg[0] == 'W') {
				printf("Set Mode: Write\n");
				mode = 'W'; //set to write mode
				done = 0;
				break;
			}

			if (strg[0] == 'V'){
				printf("Set Mode: Verify\n");
				mode = 'V';//set to verify mode
				done = 0;
				break;
			}

				if (done == 0){
					InData.addr = toHex(strg); //convert addr string to hex
					printf("Address: %x\n", InData.addr); //print address value received
					done = -1; //address accepted, don't run this again until next cycle
					break;
				}

				else{
					InData.data = toHex(strg); //convert data into string
					printf("Data: %x\n", InData.data); //print data value received
					done = 1; //address and data received, ready to be written
					break;
				}

			}

			chr = getchar(); //get following char
		}

		if(done == 1){
			if(mode == 'W'){
				writeBits_16(InData.addr, InData.data); //write data to addr on EEPROM
				done = 0; //ready to take another set of inputs
				printf("Wrote: %x to addr: %x\n", InData.data, InData.addr); //print for confirmation
			}

			else if (mode == 'V'){
				addr_shiftOut(InData.addr);
				uint16_t eeprom_data = shiftIn(1);
				printf("%x\n", eeprom_data);
				if (eeprom_data != InData.data) printf("ERROR: Data in EEPROM is: %x\n", eeprom_data);
				if(errors) printf("Found %d Errors", errors);
				done = 0;
			}

			else printf("Mode Not Set!");
		}

	}


	return 0;//should never get here

}


int addr_shiftOut(uint16_t data) {

	pulsePin_us(SRCLR, 1);

	delay_128ns();

	for(int i = 0; i <= 14; i++){
    gpio_put(SER_OUT, (data >> i) & 0x01);
    sleep_us(HOLD_US);
    gpio_put(SFT_CLK, 1);
    sleep_us(SFT_PER);
    gpio_put(SFT_CLK, 0);
    sleep_us(SFT_PER);
  }

  gpio_put(R_CLK, 1);
  sleep_us(SFT_PER);
  gpio_put(R_CLK, 0);

  return 0;
}

int data_shiftOut(uint16_t data){
	for(int i = 0; i <= 15; i++){
    gpio_put(D_SER_OUT, (data >> i) & 0x01);
    sleep_us(HOLD_US);
    gpio_put(D_SFT_CLK, 1);
    sleep_us(SFT_PER);
    gpio_put(D_SFT_CLK, 0);
    sleep_us(SFT_PER);
  }

  gpio_put(D_R_CLK, 1);
  sleep_us(SFT_PER);
  gpio_put(D_R_CLK, 0);

  return 0;
}

void pulsePin_us(int pin, int delay){
	if(gpio_get_out_level(pin)) { //Might cause race condition. Consider using gpio_get()
		gpio_put(pin, 0);
		sleep_us(delay);
		gpio_put(pin, 1);
	}
	else {
		gpio_put(pin, 1);
		sleep_us(delay);
		gpio_put(pin, 0);
	}
}

void toBin(uint16_t hexIn){
	for(int i=0; i < 16; i++) printf("%x", (hexIn >> i) & 0x0001);
	printf("\n");
}

uint16_t toHex(char * stringIn){
	uint16_t hexData = 0;
	for (int i = 0; i < 4; i++){
		if (stringIn[i] >= 97) stringIn[i] = stringIn[i] - 32; //make any uppercase letters to lowercase
		if(stringIn[i] >= 48 && stringIn[i] <= 57)
		  hexData |= (stringIn[i] - 48) << (12-(i*4));
		else if (stringIn[i] >= 65 && stringIn[i] <= 70)
			hexData |= (stringIn[i] - 55) << (12-(i*4));
		else hexData |= 0;
	}
	return hexData;
}

uint16_t shiftIn(int debug){
	uint16_t hexIn = 0x0000;
	uint16_t hexData = 0x0000;
	u_int16_t hexOut = 0;

	//setDataDir(0);

	gpio_put(CLK_EN, 1);
	gpio_put(PARALLEL_LOAD, 0);
	sleep_ms(HOLD_US);
	gpio_put(PARALLEL_LOAD, 1);

	sleep_ms(HOLD_US);

	gpio_put(CLK_EN, 0);
	// gpio_put(CLK,1);
	// sleep_ms(5);
	// gpio_put(CLK,0);
	// sleep_ms(5);
	if(debug) printf("Serial Data In Pos 0: %x\n", gpio_get(SER_IN));
	hexIn = (hexIn | gpio_get(SER_IN));
	//printf("Hex In: %x\n", hexIn);

	for(int i = 0; i<15; i++){

		gpio_put(CLK_SFT_IN,1);
		sleep_ms(SFT_PER);
		gpio_put(CLK_SFT_IN,0);
		sleep_ms(SFT_PER);

		if(debug) printf("Serial Data In Pos %d: %x\n", i + 1, gpio_get(SER_IN));
		hexIn = (hexIn << 1) | gpio_get(SER_IN);
	}

	for(int i = 0; i < 16; i++) hexOut |= (((hexIn >> i) & 0x0001) << 15 - i);

	return hexOut;
}

void setDataDir(int dir){
	if(dir == 1) {//Shift Reg enable/Disable EEPROM
		gpio_put(D_SHF_EN, 0);
		gpio_put(OUTPUT_EN, 1);
	}
	else {
		gpio_put(D_SHF_EN, 1); //Shift Reg Disable/ Enable EEPROM
		gpio_put(OUTPUT_EN, 0);
	}
}

void writeBits_16(uint16_t addr, uint16_t data){

	sleep_us(1);

	addr_shiftOut(addr);
	data_shiftOut(data);

	setDataDir(1);
	gpio_put(CHIP_SEL, 0);

	delay_64ns();

	gpio_put(WRITE_EN, 0);

	delay_128ns();

	gpio_put(WRITE_EN, 1);

	delay_64ns();

	gpio_put(OUTPUT_EN, 0);
	gpio_put(CHIP_SEL, 1);

	delay_64ns();//may not be needed

}

void delay_64ns(){
	 __asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n ");
}

void delay_128ns(){
	delay_64ns();
	delay_64ns();
}

void InitGPIO(){
	gpio_init(LED_PIN); //Set LED Pin to output
	gpio_set_dir(LED_PIN, GPIO_OUT);
	gpio_put(LED_PIN, 0);

	gpio_init(SER_OUT); //Set SER_OUT pin to output
	gpio_set_dir(SER_OUT, GPIO_OUT);

	gpio_init(SFT_CLK); //Set SFT_CLK to output
	gpio_set_dir(SFT_CLK, GPIO_OUT);

	gpio_init(SRCLR); //Set SRCLR to output
	gpio_set_dir(SRCLR, GPIO_OUT);
	gpio_put(SRCLR, 1);

	gpio_init(R_CLK); //SEt R_CLK to output
	gpio_set_dir(R_CLK, GPIO_OUT);

	gpio_init(D_SER_OUT); //Set SER_OUT pin to output
	gpio_set_dir(D_SER_OUT, GPIO_OUT);

	gpio_init(D_SFT_CLK); //Set SFT_CLK to output
	gpio_set_dir(D_SFT_CLK, GPIO_OUT);

	gpio_init(D_SRCLR); //Set SRCLR to output
	gpio_set_dir(D_SRCLR, GPIO_OUT);
	gpio_put(D_SRCLR, 1);

	gpio_init(D_R_CLK); //SEt R_CLK to output
	gpio_set_dir(D_R_CLK, GPIO_OUT);

	gpio_init(D_SHF_EN);
	gpio_set_dir(D_SHF_EN, GPIO_OUT);

	gpio_init(CLK_SFT_IN);
	gpio_set_dir(CLK_SFT_IN, GPIO_OUT);

	gpio_init(CLK_EN);
	gpio_set_dir(CLK_EN, GPIO_OUT);

	gpio_init(PARALLEL_LOAD);
	gpio_set_dir(PARALLEL_LOAD, GPIO_OUT);

	gpio_init(SER_IN);
	gpio_set_dir(SER_IN, false);

	gpio_init(WRITE_EN);
	gpio_set_dir(WRITE_EN, GPIO_OUT);
	gpio_put(WRITE_EN, 1);

	gpio_init(OUTPUT_EN);
	gpio_set_dir(OUTPUT_EN, GPIO_OUT);
	gpio_put(OUTPUT_EN, 1);


	gpio_init(CHIP_SEL);
	gpio_set_dir(CHIP_SEL, GPIO_OUT);
	gpio_put(CHIP_SEL, 1);
}
