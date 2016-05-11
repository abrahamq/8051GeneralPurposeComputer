#include <mcs51reg.h>
#include <stdio.h> 
#include <stdint.h> 
#include <ctype.h> 

volatile int ISRvar;    // ISR counter variable.

volatile uint8_t numBitsSeen; 
volatile uint8_t message; 

__xdata __at(0x4000) const char keymap [200] = 
   {0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, '`', 0,
   0, 0 /*Lalt*/, 0 /*Lshift*/, 0, 0 /*Lctrl*/, 'q', '1', 0,
   0, 0, 'z', 's', 'a', 'w', '2', 0,
   0, 'c', 'x', 'd', 'e', '4', '3', 0,
   0, ' ', 'v', 'f', 't', 'r', '5', 0,
   0, 'n', 'b', 'h', 'g', 'y', '6', 0,
   0, 0, 'm', 'j', 'u', '7', '8', 0,
   0, ',', 'k', 'i', 'o', '0', '9', 0,
   0, '.', '/', 'l', ';', 'p', '-', 0,
   0, 0, '\'', 0, '[', '=', 0, 0,
   0 /*CapsLock*/, 0 /*Rshift*/, 0 /*Enter*/, ']', 0, '\\', 0, 0,
   0, 0, 0, 0, 0, 0, 0 /*enter*/, 0,
   0, '1', 0, '4', '7', 0, 0, 0,
   '0', '.', '2', '5', '6', '8', 0 /*esc*/, 0 /*NumLock*/,
   0, '+', '3', '-', '*', '9', 0, 0,
   0, 0, 0, 0 };

//__xdata __at (0x4000) const char keymap[128] = {'?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','q','1','?','?','?','z','s','a','w','2','?','?','c','x','d','e','4','3','?','?',' ','v','f','t','r','5','?','?','n','b','h','g','y','6','?','?','?','m','j','u','7','8','?','?','?','k','i','o','0','9','?','?','?','?','l','?','p','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?'};

__sbit __at (0xB2) clk; 
__sbit __at (0xB5) keyboardData; 

volatile __bit messageReady; 
volatile __bit started; 
volatile __bit release; 
volatile __bit shift; 

//each time clk goes low, this interrupt will be called 
void INT_ISR(void) __interrupt (0)
{

   //make sure the clk line is low 
   if( clk != 0 ){
      P1 = 0x80; //error 
      return; 
   }

   if (messageReady) {
      P1 = 0x81; 
      //return; //can't take another message yet 
   } 

   //expecting an 11 bit message 
   // start=0 data0 data1 ... data7 parityBit stopBit=1 
   //figure out where we are 
   if(!started){ //start processing these 11 bits 
      //look for a start bit on keyboardData- logic 0 
      started =1; 
      if (keyboardData == 0){
         return; //will be processed by next interrupt 
      } else {
         //P1 = 0x82; //very bad, should never happen?
         return; 
      } 
   }
   else if (numBitsSeen < 8){ //haven't gotten all the data 
      //need to put keyboardData into message[numBitsSeen-1] 
      //message ^= (-keyboardData ^ message) & (1 << (numBitsSeen-1));
      if (keyboardData){
         message |= 1 << (numBitsSeen);
      } else { //clear bit 
         message &= ~(1 << (numBitsSeen));
      } 
      numBitsSeen++; 
      return; 
   } else if (numBitsSeen == 8) {
         //parity bit 
      numBitsSeen++; 
      return; 
   } else { //Done! message holds the byte- need to check for F0, 
      started = 0; 
      numBitsSeen =0; 
      if(keyboardData == 1){
         //good stop bit 
         P1 = 0x03; 
         numBitsSeen = 0; //get ready for the next message 
      } else {
         //P1 = message; 
         P1 = 0x83; //signal error, start over 
         //numBitsSeen =0; 
         //messageReady =0; 
      } 
   }

   // press: keycode, release: F0, keycode 
   // Need to deal with F0 and keycode 
   if(message == 0xF0){
      release =1;  
      message = 0; 
      return; 
   } //handle special characters (shift) 
   else if (message == 0x12){
      if(release ==0){
         shift =1; 
      } else {
         shift =0; 
         release = 0; 
      }  
      return; 
   } else { //not a special key 
      if(release){
         release =0; 
      }else{
         messageReady =1; 
      }

   } 

}


char getchar (void) {
 char c;
 while (!RI); /* wait to receive */
 c = SBUF; /* receive from serial */
 RI = 0;
 return c;
}
 
void putchar (char c) {
 while (!TI); /* wait end of last transmission */
 TI = 0;
 SBUF = c; /* transmit to serial */
}

//prints to serial until null byte 
void print(char * inp){
   for(inp; *inp != '\0'; inp++){
      putchar(*inp); 
   }
}
void UART_Init() {
 SCON = 0x50; /* configure serial */
 TMOD = 0x20; /* configure timer */
 TH1 = 253; /* baud rate 9600 */
 TL1 = 253;
 TR1 = 1; /* enable timer */
 TI = 1; /* enable transmitting */
 RI = 0; /* waiting to receive */
}

void initKeyboard(){
   numBitsSeen = 0;
   messageReady = 0; 
   return; 
} 

void delay(){
   int i = 0; 
   for(i; i < 2000; i++){
   } 

} 
 

void main(void)
{
   char c = 0; 
   EX0 = 1;        // Set interrupt.
   //IT0 = 1;        //transition, not level activated 
   IT0 = 1; //should be level- on falling level, go to interrupt 
   EA = 1;            // Set global interrupt.
   ISRvar = 0; 

   initKeyboard(); 
   UART_Init(); 

   //ready to receive 
   //for interrupts + ps2 standard 
   __asm
      setb P3.5 ;data line high 
      setb P3.2 ;clock line high  
      __endasm;

   while(1){
      unsigned char input; 
      //char res[10]; 
      while (!messageReady)
      {
         //wait until a new message appears 
      }
      input = message; 
      //sprintf(&res[0], "%02X", message); //one for status line, one because screen starts at 1
      //print(res); 
      //if(message == 0x1C){
      //   putchar('A'); 
      //}else if (message == 0xff){
      //   putchar('='); 
      //}
      putchar(keymap[message]); 
      messageReady = 0; //let go of lock, can process another message 
      //P1 = input; 
   }
}

