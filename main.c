#include "gba.h"
//---Load textures---
#include "textures/end.c"
#include "textures/title.c"
#include "textures/bird.c"
#include "textures/pipedown.c"
#include "textures/pipeup.c"
#include "textures/background.c"

//---Global variables---
#define  GBA_SW 160                                        //actual gba screen width
#define  SW     120                                        //game screen width
#define  SH      80                                        //game screen height
#define RGB(r,g,b) ((r)+((g)<<5)+((b)<<10))                //15 bit, 0-31, 5bit=r, 5bit=g, 5bit=b 
#define NUM_PIPES 2
int frameCounter=1;                                       //for frames per second
int gameState=0;                                           //game state, title, game, ending
int endScreenTimer=0;                                      //time in second to hold on the ending
char heightPt = 0;
char heights[] = {28, 45, 52, 33, 41, 56, 24, 49, 37, 31};

typedef struct                                             //player
{
 float xact, yact; //actual position for smooth movement
 float yspd; //speed of movement
 const u16* map;//current image map
}Player; 
Player P;

typedef struct main
{
  int xact, yact; //actual position for smooth movement
  const u16* map;//current image map
}Pipe;
Pipe pipes[NUM_PIPES];


void clearBackground()
{
  int y;
  u16 color = RGB(8,12,16);
  
  for(y = 0; y < SH; y++)
  {
    int vram_offset = y * GBA_SW;
    int x;
    for(x = 0; x < SW; x++) {
        VRAM[vram_offset + x] = color;
    }
  }
}

void jump()
{
  P.yspd=-2.5;
}

void buttons()                                             //buttons to press
{
 if(KEY_R ){ }  //move right
 if(KEY_L ){ }  //move left
 if(KEY_U ){ jump();}  //move up
 if(KEY_D ){ }  //move down
 if(KEY_A ){ } 
 if(KEY_B ){ jump();} 
 if(KEY_LS){ } 
 if(KEY_RS){ } 
 if(KEY_ST){ } 
 if(KEY_SL){ } 
}

IN_IWRAM void drawImage(int w,int h, int xo,int yo, const u16* map, int to)
{
  int x, y;
    
  // if (yo >= SH || xo >= SW || xo + w <= 0 || yo + h <= 0) return;

  for(y = 0; y < h; y++) 
  {                
      int screen_y = y + yo;
      // Skip rows that are outside the visible game screen bounds
      if (screen_y < 0 || screen_y >= SH) continue;

      // Calculate the VRAM row start address once per row
      volatile u16* vram_row = &VRAM[screen_y * GBA_SW + xo];
      const u16* map_row = &map[(y + to * h) * w];

      for(x = 0; x < w; x++)
      { 
          int screen_x = x + xo;
          if (screen_x >= 0 && screen_x < SW) // Ensure we stay in game screen width
          {
              u16 c = map_row[x]; 
              if(c > 0) // Transparency check
              { 
                  vram_row[x] = c;
              }
          }
      }  
  }
}

void gameUpdates()
{
  // player movement
  P.yact+=P.yspd;
  P.yspd+=0.5; //gravity
  if(P.yact>SH-8){ P.yact=SH-7; P.yspd=0;} //floor collision
  if(P.yact<0){ P.yact=0; P.yspd=0;} //ceiling collision

  // pipe movement collision
  int i =0;
  for(i=0; i<NUM_PIPES; i++)
  {
    pipes[i].xact-=1; //move pipes left
    if(pipes[i].xact<-16) //respawn pipe on the right
    {
      pipes[i].xact=SW+16;
      pipes[i].yact=heights[heightPt];
      heightPt=(heightPt+1)%10;
    }
    else if (pipes[i].xact>3 && pipes[i].xact<27) //check collision when pipe is near player
    {
      if(P.yact>pipes[i].yact + 13 || P.yact<pipes[i].yact-13) //check if player is between pipes
      {
        gameState=2; //end game
      }
    }
  }
}

void drawPipes()
{
  int i;
  for(i=0; i<NUM_PIPES; i++)
  {
    drawImage(32,64, pipes[i].xact - 16, pipes[i].yact -80, pipedown_Map, 0);
    drawImage(32,64, pipes[i].xact - 16, pipes[i].yact +16, pipeup_Map, 0);
  }
}

typedef struct
{
 u16* song;
 int tic;
 int spd;
 int size;
 int onOff;
}Music; 
Music M[3];

u16 notes[] = 
{
   44, 157,  263, 363,  458,  547, 631,  711, 786,  856, 923,  986,//C2,C2#, D2,D2#, E2, F2,F2#, G2,G2#, A2,A2#, B2 
 1046,1102, 1155,1205, 1253, 1297,1340, 1379,1417, 1452,1486, 1517,//C3,C3#, D3,D3#, E3, F3,F3#, G3,G3#, A3,A3#, B3 	
 1547,1575, 1602,1627, 1650, 1673,1694, 1714,1732, 1750,1767, 1783,//C4,C4#, D4,D4#, E4, F4,F4#, G4,G4#, A4,A4#, B4
 1798,1812, 1825,1837, 1849, 1860,1871, 1881,1890, 1899,1907, 1915,//C5,C5#, D5,D5#, E5, F5,F5#, G5,G5#, A5,A5#, B5
 1923,1930, 1936,1943, 1949, 1954,1959, 1964,1969, 1974,1978, 1982,//C6,C6#, D6,D6#, E6, F6,F6#, G6,G6#, A6,A6#, B6
 1985,1989, 1992,1995, 1998, 2001,2004, 2006,2009, 2011,2013 ,2015,//C7,C7#, D7,D7#, E7, F7,F7#, G7,G7#, A7,A7#, B7
};

u16 song_1[]={ 10,0,10, 0, 3,2,3,2, 7,22,0,8, 9,24,0,12};          //title song
u16 song_2[] = { 14, 14, 26, 0, 23, 0, 0, 22,0, 21, 0, 19, 0, 14, 19, 21,12, 12, 26, 0, 23, 0, 0, 22,0, 21, 0, 19, 0, 14, 19, 21 };
u16 song_3[]={ 60,58,56,54, 52,50,48,46, 44,42,40,38, 36,34,32,30};//end   song

void playSong(int s, int loop)
{
 if((frameCounter % (M[s].spd))==0 && M[s].onOff==1) 
 {
  int note=M[s].song[M[s].tic];
  if(note>0){ PlayNote(notes[note],64);}
  M[s].tic+=1; if(M[s].tic>M[s].size){ M[s].tic=0; if(loop==0){ M[s].onOff=0;}}
 }
}//-----------------------------------------------------------------------------

  void init()
  {
    P.xact=15; P.yact=15; P.map=bird_Map; //init player
    endScreenTimer=0; //clear timer
    //init music
    M[0].song=song_1; M[0].spd=4; M[0].tic=0; M[0].size=15; M[0].onOff=1;
    M[1].song=song_2; M[1].spd=4; M[1].tic=0; M[1].size=15; M[1].onOff=1;
    M[2].song=song_3; M[2].spd=1; M[2].tic=0; M[2].size=15; M[2].onOff=1;

    Pipe p1; p1.xact=212; p1.yact=40;
    pipes[0]=p1;

    Pipe p3; p3.xact=136; p3.yact=56;
    pipes[1]=p3;
  }

int main()
{
 *(u16*)0x4000000 = 0x405;
 REG_BG2PA=256/2; //256=normal 128=scale
 REG_BG2PD=256/2; //256=normal 128=scale 
  
 init();                                                             //init game variables

 while(1) 
 { 
    while(*(volatile u16*)0x04000006 < 160);

    if (DISPCNT & BACKB) 
    { 
       DISPCNT &= ~BACKB; VRAM = (u16*)VRAM_B; 
    } else 
    {
       DISPCNT |=  BACKB; VRAM = (u16*)VRAM_F; 
    }

    if(gameState==0)  //title screen
    {  
      playSong(0,1); //play title song
      drawImage(120,80, 0,0, title_Map, 0); //draw title screen
      if(KEY_STATE != 0x03FF){ init(); gameState=1;} //any button pressed
    } 

    if(gameState==1)  //play the game-------------------------------------------- 
    {
      drawImage(120,80, 0,0, background_Map, 0); //draw title screen background
      playSong(1,1);
      buttons();
      gameUpdates(); //Buttons pressed  
      drawPipes(); //draw pipes
      drawImage(16,14, ((int)P.xact) - 8,((int)P.yact) - 7, bird_Map, 0); // draw bird
    } 

    if(gameState==2)  //end screen-----------------------------------------------
    { 
      playSong(2,0);                                                   //play end song once
      drawImage(120,80, 0,0, end_Map, 0);                              //draw end screen
      endScreenTimer+=1; if(endScreenTimer>30*2){ gameState=0;}        //hold for 3 seconds
    }  	
    while(*(volatile u16*)0x04000006 < 160); 
    frameCounter+=1;
    if(frameCounter>120){ frameCounter=1;}
  }
}
