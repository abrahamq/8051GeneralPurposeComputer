#include <mcs51/8051.h>
#include <stdio.h>
#include <ctype.h>

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

#define width 16
#define height 30

//num rows,num columns 
extern __xdata __at (0x5000) unsigned char volatile text[height][width]; // = { 0}; //initialize all to null bytes  
__xdata __at (0x5000) unsigned char volatile text[height][width]; // = { 0}; //initialize all to null bytes  
//char text[height][width]; // = { 0}; //initialize all to null bytes  

extern  int volatile cursorX; 
int volatile cursorX = 0; 

extern int volatile cursorY; 
int volatile cursorY = 0; 

extern int volatile maxX; 
extern int volatile maxY; 

extern int volatile normalMode; //if one, in esc, otherwise in insert mode 

int volatile maxX = 0; 
int volatile maxY = 0; 

int volatile normalMode = 0; //if one, in esc, otherwise in insert mode 

 
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
  if ( cursorX > maxX){
     maxX = cursorX ;
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

void backspace(){
   text[cursorY][cursorX] = 0; 
   putchar('\0'); //overwrite current position on screen 
   cursorX--; 
   if(cursorX < 0){
      if(cursorY != 0){ //if cursorY is equal to zero, nothing to do 
         cursorY--;
         cursorX = -1*cursorX;
      }
      else{
         cursorX = 0; 
      }
   }
   putchar(8); //backspace
   putchar(8); //backspace
}

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
   int i=0; 
   int j=0; 
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
   moveCursor(&res[0] , "0"); //move to beginning of line 
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
   } 
}

void updateStatusLine(){
   int copyY = cursorY;
   int copyX = cursorX;
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
   int i=0; 
   int j=0; 
   for(j; j< height; j++){
      for(i; i<width; i++){
         text[j][i] = '\0'; 
      }
      i = 0; 
   }
}
 
void main() {
   P1 = 0x55; 
 UART_Init();
 //init(); 
 clear(); 
 updateStatusLine(); 
 crlf(); //make way for status line 
 cursorY = 0; 
 cursorX = 0; 
 moveCursor("2", "0"); 
 for(;;) {
  char c;
  c = getchar();

  //update the status line: 
  updateStatusLine(); 

  handleInput(c); 

  //update the status line: 
  updateStatusLine(); 
 }
}
