#include <mcs51/8051.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

//
//TODO: 
//enter works 
//esc/insert mode 
//	hjkl mode 
//	dd 
//	$
//	x 
//
//maybe: 
//backspace works 

//for keyboard 
volatile int ISRvar;    // ISR counter variable.

volatile uint8_t numBitsSeen; 
volatile uint8_t message; 

__sbit __at (0xB2) clk; 
__sbit __at (0xB5) keyboardData; 

volatile __bit messageReady; 
volatile __bit started; 
volatile __bit release; 
volatile __bit shift; 

__xdata __at(0x4000) const unsigned char keymap [200] = 
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


#define width 16
#define height 30

__xdata __at (0x4500)  uint8_t volatile maxCol[height]; 

//num rows,num columns 
extern __xdata __at (0x5000) unsigned char volatile text[height][width] = {{ 0}}; //initialize all to null bytes  
__xdata __at (0x5000) unsigned char volatile text[height][width]; // = { 0}; //initialize all to null bytes  

extern  uint8_t volatile cursorX; 
uint8_t volatile cursorX = 0; 

extern uint8_t volatile cursorY; 
uint8_t volatile cursorY = 0; 

uint8_t  volatile maxX; 
uint8_t  volatile maxY; 

extern uint8_t volatile normalMode; //if one, in esc, otherwise in insert mode 
uint8_t volatile normalMode = 0; //if one, in esc, otherwise in insert mode 


//interrupt for keyboard
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


void initKeyboard(){
   numBitsSeen = 0;
   messageReady = 0; 
   EX0 = 1;        // Set interrupt.
   //IT0 = 1;        //transition, not level activated 
   IT0 = 1; //should be level- on falling level, go to interrupt 
   EA = 1;            // Set global interrupt.
   ISRvar = 0; 

   __asm
      setb P3.5 ;data line high 
      setb P3.2 ;clock line high  
      __endasm;

   return; 
} 


//function prototypes; 
void goToStartOfLine(); 

char keybuff[4]; //holds last 4 keystrokes 
uint8_t keysInBuffer= 0; 

char getchar(void){
   unsigned char input; 
   while (!messageReady)
   {
      //wait until a new message appears 
   }
   input = message; 
   messageReady = 0; //let go of lock, can process another message 

   return (keymap[message]); 
}
 
//char getchar (void) {
// char c;
// while (!RI); /* wait to receive */
// c = SBUF; /* receive from serial */
// RI = 0;
// return c;
//}
 
void putchar (char c) {
 while (!TI); /* wait end of last transmission */
 TI = 0;
 SBUF = c; /* transmit to serial */
}

void crlf(){
   putchar(13); //CR 
   putchar(10); //LF 
   return; 
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

//writes ^[[ then c 
void writeAnsiEsc(char c ){
   putchar(27); 
   putchar('['); 
   putchar(c); 
}

void up(){
   if(cursorY == 0){
      return; 
   }
   cursorY--; 
   writeAnsiEsc('A'); 
}

void down(){
   if(cursorY == height-1){
      return; 
   }
   cursorY++; 
   writeAnsiEsc('B'); 
}

void left(){
   if(cursorX == 0){
      return; //beginning of line, do nothing
   }
   cursorX--; 
   //go left 
   writeAnsiEsc('D'); 
}


void right(){
  if(cursorX == width-1){
     return; //end of line, do nothing 
  }; 
  cursorX++; 
  cursorX = cursorX%width;
  writeAnsiEsc('C'); 
}



//Inserts the argument into the text array, then 
//updates the screen to show just the addition of that character
//inserts a crlf when cursorX = width-1 
void insertChar(char c){
   char * endOfText; 
  if(cursorX == width-1){//end of line, do nothing
     //right(); 
     //down(); 
  }else{
     text[cursorY][cursorX] = c; 
     endOfText = &text[cursorY][cursorX];
     endOfText++; 
     *endOfText = '\0'; //null ptr at end of string 
     
     putchar(c); 
     cursorX++; 
     //right(); 
  }
  if ( cursorY > maxY){
     maxY = cursorY ;
  }
  if ( cursorX > maxCol[ cursorY]){
     maxCol[cursorY] = cursorX ;
  }
}


void saveCursor(){
   putchar(27); 
   putchar('['); 
   putchar('s'); 
}

void restoreCursor(){
   putchar(27); 
   putchar('['); 
   putchar('u'); 
}

//clears line in terminal, replaces all 
//in terminal buffer with /00 
//sets cursor to beginning of line 
//TODO: erase line in text 
void d(){
   writeAnsiEsc('2'); 
   putchar('K'); 
}

void printLine(int row){
   uint8_t i; 
   uint8_t saveX = cursorX; 
   saveCursor(); 
   goToStartOfLine(); 
   for (i =0; i< maxCol[row]; i++){
      putchar(text[row][i]); 
   }
   for (i =maxCol[row]; i<width; i++){
      putchar(' '); 
   }
   restoreCursor();
   cursorX = saveX; 
}

void backspace(){
   uint8_t i=0;
   for(i = cursorX; i< width-1; i++){ //copy everything from cursorX to width into cursorX-1 in text 
      text[cursorY][i] = text[cursorY][i+1]; 
   }
   printLine(cursorY); 
   text[cursorY][width-1] = ' '; 
   //TODO: update maxCols- to erase 
}

//void backspace(){
//   moveCursor(cursorX-1, cursorY);
//   putchar(' '); //overwrite current position on screen 
//   text[cursorY][cursorX] = 0; 
//   cursorX--; 
//   if(cursorX < 0){
//      if(cursorY != 0){ //if cursorY is equal to zero, nothing to do 
//         cursorY--;
//         cursorX = -1*cursorX;
//      }
//      else{
//         cursorX = 0; 
//      }
//   }
//   putchar(8); //backspace
//   putchar(8); //backspace
//}

//prints to serial until null byte 
void print(char * inp){
   for(inp; *inp != '\0'; inp++){
      putchar(*inp); 
   }
}

//moves the onscreen cursor to x,y 
//also moves the cursor in the rep 
void moveCursor(char * vertical, char * horizontal){
   putchar(27); 
   putchar('['); 
   print(vertical); 
   putchar(';'); 
   print(horizontal); 
   putchar('H'); 
   //cursorX = horizontal; 
   //cursorY = vertical; 
}

extern struct cursorPos
{
   char vertical;
   char horizontal;
};

//doesn't work 
//void getCursorPos(struct cursorPos* result  ) {
//
//   putchar(27);
//   putchar('[');
//   putchar('6');
//   putchar('n');
//   //response = ^[<v>;<h>R
//
//   getchar(); // throw away ^[ 
//   result->vertical = getchar(); 
//   getchar(); // throw away  ;
//   result->horizontal = getchar(); 
//   getchar(); // throw away  R
//   return; 
//}

void drawAll(){
   uint8_t i=0; 
   uint8_t j=0; 
   saveCursor(); 
   moveCursor("30", "0"); 
   putchar('S'); 
   putchar(text[j][i]); 
   crlf(); 
   for(j; j< 10; j++){
      for(i; i<width; i++){
         putchar(text[j][i]); 
      }
      crlf(); 
      i = 0; 
   }
   restoreCursor(); 
}

void clear(){
   putchar(27); 
   putchar('['); 
   putchar('2'); 
   putchar('J'); 
}

void handleEsc(){
   char throwaway = getchar();
   if (throwaway == '['){
      char direction = getchar(); 
      if(direction == 'D'){
         left(); 
      }
      else if (direction == 'C'){ //right 
         right(); 
      } 
      else if (direction == 'A'){ //up 
         up(); 
      } 
      else if (direction == 'B'){ //down 
         down(); 
      } 
   } else if (throwaway == 'M') { //enter key 
      insertChar(13); //TODO is this a mistake?- shouldn't an enter key take us to the next line, not just enter a cr? 
   } else { //just an esc key press- go into esc mode 
      normalMode = 1; 
   } 
}

void goToStartOfLine(){
   char res[10]; 
   sprintf(&res[0], "%d", cursorY+2); //one for status line, one because screen starts at 1
   cursorX = 0; 
   moveCursor(&res[0] , "1"); //move to beginning of line 
}

void normalModeHandler(char direction){
   if (direction == 'i'){
      normalMode = 0; 
      return; 
   }
   if(direction == 'r'){ //redraw 
      drawAll(); 
   }
   else if(direction == 'c'){ //clear screen 
      clear(); 
   }
//   else if(direction == 'f'){ //find cursor position 
//      struct cursorPos result; 
//      struct cursorPos* ptr_result = &result; 
//      getCursorPos(ptr_result);
//      putchar(result.vertical); 
//      putchar(';'); 
//      putchar(result.horizontal); 
//   }
   else if(direction == '0'){ //beginning of line 
      goToStartOfLine(); 
   }
   else if(direction == 'h'){
      left(); 
   }
   else if (direction == 'l'){ //right 
      right(); 
   } 
   else if (direction == 'k'){ //up 
      up(); 
   } 
   else if (direction == 'j'){ //down 
      down(); 
   } else if (direction == 'x'){
      backspace(); 
   } 
}

void updateStatusLine(){
   uint8_t copyY = cursorY;
   uint8_t copyX = cursorX;
   char resY[10]; 
   char resX[10]; 
   //save cursor on terminal: 
   saveCursor(); 
   sprintf(&resY[0], "%d", cursorY); 
   sprintf(&resX[0], "%d", cursorX); 
   moveCursor("0", "0"); 
   print("CursorY: "); 
   print(&resY[0]); 
   print(" CursorX: "); 
   print(&resX[0]); 
   //restore cursor 
   //moveCursor(&resY[0] , &resX[0]); //ascii 
   restoreCursor(); 
}

void handleEnter(){
   insertChar(13); 
   insertChar(10); 
   cursorX = 0; 
   if( cursorY != height-1){
      cursorY++; 
   } 
}


void handleInput(char c){
  if(normalMode == 1){
     normalModeHandler(c);
     return;
  }

  if(c == 13){
     handleEnter(); 
     return; 
  }

  if(c == 27){ //an escape code, not alphanumeric 
     handleEsc(); 
     return; 
  }

  if(c >= 32 && c < 127 && normalMode == 0){ //alphanumeric 
     insertChar(c); 
  }
  else if(c == 0x08){ //backspace 
     backspace(); 
  }
}

void init(){
   uint8_t i=0; 
   uint8_t j=0; 
   for(j=0; j< height; j++){
      for(i=0; i<width; i++){
         text[j][i] = '\0'; 
      }
   }
}
 
void main() {
   P1 = 0x55; 
 UART_Init();
 initKeyboard(); 
 clear(); 
 init(); //TODO: be careful- we want to be able to load files? 
 updateStatusLine(); 
 crlf(); //make way for status line 
 cursorY = 0; 
 cursorX = 0; 
 moveCursor("2", "0"); 
 for(;;) {
  char c;
  c = getchar();
  P1 = c; 

  //update the status line: 
  updateStatusLine(); 

  handleInput(c); 

  //update the status line: 
  updateStatusLine(); 
 }
}
