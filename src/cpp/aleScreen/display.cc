#include <time.h>
#include <SDL.h>
#include <stdlib.h>
#include <TPG.h>
#include <ale_interface.hpp>
#include <unistd.h>
#include "ALEScreenEx.cc"

SDL_Surface *sdl_screen;
float time_measure = 0.0f;
float time_step = 0.0f;

void draw(ALEScreenEx &bscreen,ColourPalette &palette)
{
   int i,bpp,rank,x,y,r,g,b;
   Uint32 *pixel;
   SDL_LockSurface(sdl_screen);
   rank = sdl_screen->pitch/sizeof(Uint32);
   pixel = (Uint32*)sdl_screen->pixels;
   /* Draw all dots */
   for(y = 0;y < bscreen.height(); y++){
      for (x = 0; x < bscreen.width(); x++){
         int colour = bscreen.get(y,x);
         palette.getRGB(colour,r,g,b);
         pixel[x+y*rank] = SDL_MapRGBA(sdl_screen->format,r,g,b,255);
      }
   }
   SDL_UnlockSurface(sdl_screen);
}

/* Convert from timespec to float */
float convert_time(struct timespec *ts)
{
   float accu;
   /* Combine the value into floating number */
   accu = (float)ts->tv_sec; /* Seconds that have gone by */
   accu *= 1000000000.0f; /* One second is 1000x1000x1000 nanoseconds, s -> ms, us, ns */
   accu += (float)ts->tv_nsec; /* Nanoseconds that have gone by */
   /* Bring it back into the millisecond range but keep the nanosecond resolution */
   accu /= 1000000.0f;
   return accu;
}

/* Start time */
void start_time()
{
   struct timespec ts;
   clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&ts);
   time_measure = convert_time(&ts);
}

/* End time */
void end_time()
{
   struct timespec ts;
   float delta;
   clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&ts);
   delta = convert_time(&ts)-time_measure; /* Find the distance in time */
   time_step = delta/(1000.0f/16.0f); /* Weird formula, equals 1.0f at 16 frames a second */
}

int main(int argc, char** argv) {
   char * frameFile;
   int option_char;
   while ((option_char = getopt (argc, argv, "r:")) != -1)
      switch (option_char)
      {
         case 'r': frameFile = optarg; break;
         case '?':
                   cout << "-r <ROM> (Atari 2600 ROM file, named as per ALE supported ROMs)" << endl;
                   exit(0);
      }
   ColourPalette palette;
   palette.setPalette("standard","NSTC");
   SDL_Event ev;
   int active;
   /* Initialize SDL */
   if(SDL_Init(SDL_INIT_VIDEO) != 0)
      fprintf(stderr,"Could not initialize SDL: %s\n",SDL_GetError());
   /* Open main window */
   sdl_screen = SDL_SetVideoMode(ALE_SCREEN_WIDTH,ALE_SCREEN_HEIGHT,0,SDL_HWSURFACE|SDL_DOUBLEBUF);
   if(!sdl_screen)
      fprintf(stderr,"Could not set video mode: %s\n",SDL_GetError());
   ALEScreenEx frame(ALE_SCREEN_HEIGHT,ALE_SCREEN_WIDTH);
   //char romBackground[80]; sprintf(romBackground, "%s/%s%s","backgrounds/",rom,".bg");
   frame.load(frameFile);
   /* Main loop */
   active = 1;
   while(active)
   {
      /* Handle events */
      while(SDL_PollEvent(&ev))
      {
         if(ev.type == SDL_QUIT)
            active = 0; /* End */
      }
      /* Start time */
      //start_time();
      /* Clear screen */
      SDL_FillRect(sdl_screen,NULL,SDL_MapRGBA(sdl_screen->format,0,0,255,255));
      /* Draw game */
      draw(frame,palette);
      /* Show screen */
      SDL_Flip(sdl_screen);
      /* End time */
      //end_time();
   }
   /* Exit */
   SDL_Quit();
   return 0;
}
