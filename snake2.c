#include "address_map_arm.h"
#include <stdio.h>
#include <stdbool.h>ü
#include "eatSound.h"
#include "dieSound.h"

#define startLength 4;
#define BLUE 0x187F
#define RED 0xF800
#define GREEN 0x05F0
#define BLACK 0x0000
#define frameUp 10
#define frameDown 220
#define frameRight 310
#define frameLeft 10
#define maxSize 200
#define velocity 3
#define squareSize 2

#define clock 80

#define scoreX 12 
#define scoreY 57
#define nameX 60 
#define nameY 57

#define BUF_THRESHOLD 96		


void game5();


void moveRight();
void moveLeft();
void moveUp();
void moveDown();
void checkADC();

/* function prototypes */
void printText (int, int, char *);
void drawFrame();
void drawPixel(int,int, short);
void removePixel(int,int);
void initializeSnake2();
void updateSnake();
void drawSnake2();void flushScreen();
void delay(int time);
void drawSquare(int,int,int,short);
void removeSquare(int, int, int);
void bait();
bool checkEat();
void ateIt();
void sound(int*,int, bool);
void checkDead();
bool checkIfAtSnakeHead(int, int);
void deadScene();
void printScore();




void set_A9_IRQ_stack (void);
void config_GIC (void);
void config_HPS_timer (void);
void config_PS2(void);
void enable_A9_interrupts (void);

struct body
{
    int x;
    int y;
    int exist;
};

struct body snake[maxSize];
int xBait;
int yBait;
volatile int tick = 0;					// set to 1 every time the HPS timer expires
int vX= velocity;
int vY = 0;
int score = 0;
int length = startLength;
bool isDead = false;
bool adcOn = false;





int main(void)
{
	set_A9_IRQ_stack ();			// initialize the stack pointer for IRQ mode
	config_GIC ();					// configure the general interrupt controller
	config_HPS_timer ();			// configure the HPS timer
	config_PS2();
	enable_A9_interrupts ();	// enable interrupts
	
	game5();
}
	
void game5()
{
	flushScreen();
	initializeSnake2();
	drawFrame();
	
	char text1[40] = "Kaan Oktay";
	char text2[40] = "Ahmet Baglan";
	char text3[40] = "Cemre Imer";
	char text4[40] = "Score:";
	char text5[40] = "ADC OFF";
	
	printText(nameX, nameY, text1);
	printText(nameX, nameY+1, text2);
	printText(nameX, nameY+2, text3);
	printText(scoreX, scoreY, text4);
	printText(1, scoreY, text5);
	
	printScore();
	bait();
	
	while(1)
	{
		if(adcOn)
		{
			checkADC();
		}
		
		delay(clock);
		drawSnake2();
		delay(clock);
		updateSnake(vX,vY);
		removeSquare(snake[length].x,snake[length].y,squareSize);
		/*if(snake[0].x>= frameRight || snake[0].x<= frameLeft)
		{
				vX=-vX;
		}
		else if(snake[0].y>= frameDown || snake[0].y<= frameUp)
		{
				vY=-vY;
		}*/
		
		if(checkEat())
		{
			ateIt();
		}	
		checkDead();
		while(isDead)
		{
			
			deadScene();
		
		}
			delay(clock/2);
	}
		
}
	
	
	
void deadScene()
{
	adcOn = false;
	vX = 0;
	vY = 0;	
	char text[400] = "Heyyooo You Lost wanna restart press Enter";
	printText(15, 30, text);
	while(isDead)
	{
		sound(die_buffer,die_buf_size,true);
	}
	if(!isDead)
	{
		char text[400] = "                                                                                              ";
		printText(15, 30, text);	
	}
	
}
void bait()
{
	xBait = (frameLeft+1)+rand()%(frameRight-frameLeft-squareSize);
	yBait = (frameUp+1)+rand()%(frameDown-frameUp-squareSize);
	drawSquare(xBait,yBait,squareSize,BLUE);
}

bool checkEat()
{
	
	if( snake[0].x > xBait - squareSize && snake[0].x < xBait+ squareSize && snake[0].y > yBait - squareSize && snake[0].y < yBait+ squareSize)
	{
	
		return true;
	}
	return false;
}

void ateIt()
{
		score+=1;
		printScore();
		removeSquare(xBait,yBait, squareSize);
		bait();
		length += 1;
		snake[length].x =  (2 * snake[length-1].x - snake[length-2].x);
		snake[length].y =  (2 * snake[length-1].y - snake[length-2].y);
		snake[length].exist = 1;		
		
		sound(eat_buffer,eat_buf_size/2,false);
}


void checkDead()
{
	
	int i;
	for(i = 1; i <length; i++)
	{
		
		if(snake[i].exist == 0)
		{
			break;
		}
		
		if(snake[i].x == snake[0].x && snake[i].y == snake[0].y)
		{
			isDead = true;
			break;
		}
	}
	
	if(snake[0].x > frameRight - squareSize || snake[0].x < frameLeft)
	{
		isDead = true;
	}
	
	else if(snake[0].y > frameDown - squareSize || snake[0].y < frameUp)
	{
		isDead = true;
	}
	
	
	
}
void delay(int time)// In terms of milisecond /
{
	int a = 0;
	 while(a<time)
	 {
		 if(tick)
		 {
			 a++;
			 tick = 0;
		 } 
	 } 
}

/* setup HPS timer */
void config_HPS_timer()
{
	volatile int * HPS_timer_ptr = (int *) HPS_TIMER0_BASE;	// timer base address

	*(HPS_timer_ptr + 0x2) = 0;		// write to control register to stop timer
	/* set the timer period */
	int counter = 100000;			// period = 1/(100 MHz) x (100 x 10^6) = 0.1 sec
	*(HPS_timer_ptr) = counter;		// write to timer load register

	/* write to control register to start timer, with interrupts */
	*(HPS_timer_ptr + 2) = 0b011;		// int mask = 0, mode = 1, enable = 1
}

void HPS_timer_ISR( )
{
	volatile int * HPS_timer_ptr = (int *) HPS_TIMER0_BASE;	// HPS timer address
	
	++tick;										// used by main program

	*(HPS_timer_ptr + 3); 					// Read timer end of interrupt register to
													// clear the interrupt
	return;
}


/* setup the PS/2 interrupts */
void config_PS2()
{
	volatile int * PS2_ptr = (int *) PS2_BASE;	// PS/2 port address

	*(PS2_ptr) = 0xFF;				/* reset */
	*(PS2_ptr + 1) = 0x1; 			/* write to the PS/2 Control register to enable interrupts */
}

void PS2_ISR ()
{
	volatile char byte1, byte2, byte3;
	volatile int * PS2_ptr = (int *) 0xFF200100;		// PS/2 port address
	int PS2_data, RAVAIL;
	PS2_data = *(PS2_ptr);									// read the Data register in the PS/2 port
	RAVAIL = (PS2_data & 0xFFFF0000) >> 16;			// extract the RAVAIL field
	if (RAVAIL > 0)
	{
		/* always save the last three bytes received */
		byte1 = byte2;
		byte2 = byte3;
		byte3 = PS2_data & 0xFF;	
		if(byte1 == (char) 0x75 || byte1 == (char) 0x1D)   //See http://www.computer-engineering.org/ps2keyboard/scancodes2.html
		{
			moveUp();
			return;
		}
		else if(byte1 == (char) 0x72 || byte1 == (char) 0x1B)
		{
			moveDown();
			return;
		
		}
		else if(byte1 == (char) 0x74||byte1 == (char) 0x23)
		{
			moveRight();
			return;
		}
		else if(byte1 == (char) 0x6B||byte1 == (char) 0x1C)
		{
			moveLeft();
			return;
		}
		else if(byte1 == (char) 0x5A)
		{
			if(isDead)
			{
				isDead = false;
				flushScreen();
				initializeSnake2();
				drawFrame();
				bait();
				char text[400] = "                                                                                              ";
				printText(15, 30, text);
				
				vX = velocity;
				vY = 0;
			
				score = 0;
				printScore();
				
				length =startLength;
			}
			
			return;
		}
		else if(byte1 == (char) 0x29 && byte2 != (char) 0xF0)
		{
			adcOn = !adcOn;
			if(adcOn)
			{
				char text5[40] = "ADC ON ";
				printText(1, scoreY, text5);
			}
			else
			{
				char text5[40] = "ADC OFF";
				printText(1, scoreY, text5);
			}
			
		}
		
	}
	
	return;
	
}

void drawSquare(int cornerX, int cornerY, int size, short color)// draw a square of given size at given position whith color
{
	int i,j; 
	for(i=cornerX; i<cornerX+size;i++)
	{
		for(j = cornerY; j<cornerY+size;j++)
		{
			drawPixel(i,j,color);
		}
	}
}

void removeSquare(int cornerX, int cornerY, int size) // removes square at given position
{
	int i,j; 
	for(i=cornerX; i<cornerX+size;i++)
	{
		for(j = cornerY; j<cornerY+size;j++)
		{
			drawPixel(i,j,BLACK);
		}
	}
	
}

void initializeSnake2()
{
	int a, b;
	 
	for (b=0;b<length;b++)
	{
		snake[b].x = 110-b*(squareSize+1);
        snake[b].y = 70;
        snake[b].exist = 1;  
	}
     
	for(a = length; a < maxSize; a++)
    {
        snake[a].x = 0;
        snake[a].y = 0;
        snake[a].exist = 0;     
    }
}

void drawSnake2()
{
	int i;
	for(i = 0; i <length; i++)
	{
		if(snake[i].exist == 0)
		{
			break;
		}
		drawSquare(snake[i].x,snake[i].y,squareSize,GREEN);
	}
}

void updateSnake(int xV, int yV)//Her cagrildiginda yilanin koordinatlarini hizina gore degistirio
{
	int i;
	
	for(i = length; i >0; i--)
	{
		snake[i].x = snake[i-1].x;	
		snake[i].y = snake[i-1].y;	
	}
	snake[0].x += xV;
	snake[0].y += yV;
}

void drawPixel(int x, int y, short pixel_color)//ekranda bi koordinata renk basio
{
	int pixel_ptr;	
	pixel_ptr = FPGA_ONCHIP_BASE + (y << 10) + (x << 1);
	*(short *)pixel_ptr = pixel_color;		// set pixel color
}

void removePixel(int x, int y)//ekranda bi koordinata renk basio
{
	int pixel_ptr;	
	pixel_ptr = FPGA_ONCHIP_BASE + (y << 10) + (x << 1);
	*(short *)pixel_ptr = BLACK;		// set pixel color
}

void drawFrame()//ekranda kirmizi cerceve basar
{
	int i; 
	for(i = frameLeft; i<frameRight;i++)
	{
		drawPixel(i,frameUp,RED);
		drawPixel(i,frameDown,RED);
	}
	
	for(i = frameUp; i<frameDown;i++)
	{
		drawPixel(frameLeft,i,RED);
		drawPixel(frameRight,i,RED);
	}
	
}

void flushScreen()//ekrani temizler
{
	int i,j;
	int monitorRight = 319;
	int monitorDown = 239;
	for(i = 0; i<=monitorRight;i++)
	{
		for(j = 0; j<=monitorDown;j++)
		{
			drawPixel(i,j,BLACK);
		}
	}
	
}

void printText(int x, int y, char * text_ptr)//Ekrana yazi yazar
{
	int offset;
  	volatile char * character_buffer = (char *) FPGA_CHAR_BASE;	// VGA character buffer

	/* assume that the text string fits on one line */
	offset = (y << 7) + x;
	while ( *(text_ptr) )
	{
		*(character_buffer + offset) = *(text_ptr);	// write to the character buffer
		++text_ptr;
		++offset;
	}
}

void sound(int win_buffer[], int BUF_SIZE, bool deadLoop)
{

	volatile int * audio_ptr = (int *) AUDIO_BASE;

	/* used for audio record/playback */
	int fifospace;
	int play = 1, buffer_index = 0;
	int left_buffer[BUF_SIZE];
	int right_buffer[BUF_SIZE];
	 	
	while (play)
		{
			fifospace = *(audio_ptr + 1);	 		// read the audio port fifospace register
			if ( (fifospace & 0x00FF0000) > BUF_THRESHOLD ) 	// check WSRC
			{
				// output data until the buffer is empty or the audio-out FIFO is full
				while ( (fifospace & 0x00FF0000) && (buffer_index < BUF_SIZE))
				{
					*(audio_ptr + 2) = win_buffer[buffer_index];
					*(audio_ptr + 3) = win_buffer[buffer_index];
					++buffer_index;

					if (buffer_index == BUF_SIZE)
					{
						play = 0;
					}
					
					if(deadLoop)
					{
						if(!isDead)
						{
							play = 0;
							break;
						}
					}
					
					fifospace = *(audio_ptr + 1);	// read the audio port fifospace register
				}
			}
		}
}

void printScore()
{
	
	char buf[5];
	sprintf(buf, "%d", score);
	printText(25, 57, buf);
	
}


void checkADC()
{
	volatile int * ADC_ptr = (int *) ADC_BASE;		// ADC port address
	int x,y;
	
	*(ADC_ptr + 1) = 1;	// Sets the ADC up to automatically perform conversions.
	x = *(ADC_ptr);
	y = *(ADC_ptr+1);
	
	
	//DO IF
	
	if(x>1080 && x<1500 && y> 2120 && y< 2520)
	{
		moveUp();
	}
	else if(x> 2000 && x< 2400 && y> 2520 && y< 3500)
	{
		moveLeft();
	}
	else if(x> 2000 && x< 2400 && y> 1200 && y< 2120)
	{
		moveRight();
	}
	else if(x> 2400 && x<3200 && y> 2120 && y< 2520)
	{
		moveDown();
	}
	
	
		
}

void moveRight()
{
	if(!(snake[1].x == snake[0].x + velocity && snake[1].y == snake[0].y))
		{
			vX = velocity;
			vY = 0;
		}
}

void moveLeft()
{
	if(!(snake[1].x == snake[0].x - velocity && snake[1].y == snake[0].y))
		{
			vX = -velocity;
			vY = 0;
		}
}

void moveUp()
{
	if(!(snake[1].x == snake[0].x && snake[1].y == snake[0].y - velocity))
		{
			vX = 0;
			vY = -velocity;
		}
}

void moveDown()
{
	if(!(snake[1].x == snake[0].x && snake[1].y == snake[0].y + velocity))
		{
			vX = 0;
			vY = velocity;
		}
}

