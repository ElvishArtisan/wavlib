/* ameter.c
 *
 *  An ncurses-based audio metering utility
 *
 *   (C) Copyright 2002-2005 Fred Gleason <fredg@salemradiolabs.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <strings.h>
#include <unistd.h>
#include <sys/soundcard.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <curses.h>
#include <signal.h>
#include <string.h>

#include "wavlib.h"

#define BUFFER_SIZE 16384
#define SAMPLE_RATE 44100
#define USAGE "ameter [-l <ref-level>][-t <thres-level>][-v] <dsp-dev>\n"
#define DEFAULT_THRESHOLD 12

/*
 * Function Prototypes
 */
void InitScreen(int);
void UpdateMeter(int,int,int,int);
void MeterStop(int);

/*
 * Global Variables
 */
WINDOW *wTop,*wBottom,*wLabel;
WINDOW *wLeft,*wRight;

int main(int argc,char *argv[])
{
  int i;
  int c;
  int hAudio;
  unsigned char cBuffer[BUFFER_SIZE];
  short *dBuffer;
  int dRequest;
  float fLevel=0;
  float fColor=DEFAULT_THRESHOLD;
  int dColor;
  int dRef=0;
  int dMatch=0;
  int dDebug=0;
  int dSampleRate;
  char sDevice[128];  /* Name of audio device */
  float fSampleRate;
  int dMaxLeft,dMinLeft;
  int dMaxRight,dMinRight;

  if(argc<2) {
    printf(USAGE);
    exit(0);
  }

  if(argc==2) {
    if(strcmp(argv[1],"-v")==0) {
      printf("ameter v");
      printf(VERNUM);
      printf("\n");
      exit(0);
    }
  }

  /* Get device name */
  if(sscanf(argv[argc-1],"%s",sDevice)!=1) {
    printf("ameter: invalid audio device\n");
    exit(1);
  }

  /* Get options */
  for(i=1;i<argc-1;i++) {
    dMatch=0;
    if(strcmp(argv[i],"-v")==0) {     /* Version */
      printf("\n");
      printf("ameter v");
      printf(VERNUM);
      printf("\n");
      exit(0);
    }
    if(strcmp(argv[i],"-d")==0) {     /* Debug Mode */
      dMatch=1;
      dDebug=1;
    }
    if(strcmp(argv[i],"-l")==0) {    /* Reference Level */
      dMatch=1;
      if(sscanf(argv[++i],"%f",&fLevel)!=1) {
	printf("ameter: invalid reference level\n");
	exit(1);
      }
    }
    if(strcmp(argv[i],"-t")==0) {    /* Color Threshold Level */
      dMatch=1;
      if(sscanf(argv[++i],"%f",&fColor)!=1) {
	printf("ameter: invalid color Threshold level\n");
	exit(1);
      }
    }
    if(dMatch==0) {
      printf("agen: invalid option\n");
      exit(1);
    }
  }

  /* Convert db to ratio */
  dRef=(int)(pow(10,-fLevel/20)*32768);
  dColor=(int)(pow(10,-fColor/20)*32768);

  /* Open the audio device */
  hAudio=open(sDevice,O_RDONLY);
  if(hAudio<0) {
    printf("ameter: can't open audio device\n");
    exit(1);
  }

  /* Set channelization */
  dRequest=1;     /* Stereo Mode */
  ioctl(hAudio,SNDCTL_DSP_STEREO,&dRequest);
  if(dRequest!=1) {    
    printf("ameter: can't configure channelization on %s\n",sDevice);
    close(hAudio);
    exit(1);
  }

  /* Set sample size to 16 bit */
  dRequest=AFMT_S16_LE;
  ioctl(hAudio,SNDCTL_DSP_SETFMT,&dRequest);
  if(dRequest!=AFMT_S16_LE) {
    printf("ameter: sixteen bit samples not supported on %s\n",sDevice);
    close(hAudio);
    exit(1);
  }

  /* Determine Effective Sampling Rate */
  dSampleRate=SAMPLE_RATE;
  ioctl(hAudio,SNDCTL_DSP_SPEED,&dSampleRate);
  fSampleRate=(float)dSampleRate;

  /*
   * Setup display stuff
   */
  signal(SIGINT,MeterStop);
  InitScreen((int)fLevel);

  /*
   * Main Loop
   */
  dBuffer=(short *)cBuffer;
  for(;;) {
    dMaxLeft=0;
    dMinLeft=0;
    dMaxRight=0;
    dMinRight=0;
    c=read(hAudio,cBuffer,BUFFER_SIZE);
    for(i=0;i<BUFFER_SIZE/2;i+=2) {  /* Left Channel */
      if(dBuffer[i]<dMinLeft) {
	dMinLeft=dBuffer[i];
      }
      if(dBuffer[i]>dMaxLeft) {
	dMaxLeft=dBuffer[i];
      }
    }
    if(abs(dMinLeft)>abs(dMaxLeft)) {
      dMaxLeft=abs(dMinLeft);
    }
    else {
      dMaxLeft=abs(dMaxLeft);
    }
    for(i=1;i<BUFFER_SIZE/2;i+=2) {  /* Right Channel */
      if(dBuffer[i]<dMinRight) {
	dMinRight=dBuffer[i];
      }
      if(dBuffer[i]>dMaxRight) {
	dMaxRight=dBuffer[i];
      }
    }
    if(abs(dMinRight)>abs(dMaxRight)) {
      dMaxRight=abs(dMinRight);
    }
    else {
      dMaxRight=abs(dMaxRight);
    }
    UpdateMeter(dMaxLeft,dMaxRight,dRef,dColor);
  }

  /* close files and finish */
  close(hAudio);

  exit(0);
}



void InitScreen(int dLevel)
{
  initscr();

  /*
   * Color Stuff
   */
  if(has_colors()) {
    start_color();
    init_pair(COLOR_RED,COLOR_RED,COLOR_BLACK);
    init_pair(COLOR_GREEN,COLOR_GREEN,COLOR_BLACK);
    init_pair(COLOR_WHITE,COLOR_WHITE,COLOR_BLACK);
  }

  /*
   * Create screen elements
   */
  wTop=newwin(5,80,0,0);
  wLabel=newwin(2,10,5,0);
  wLeft=newwin(1,70,5,10);
  wRight=newwin(1,70,6,10);
  wBottom=newwin(18,80,7,0);

  /*
   * Initialize Screen Labels
   *
   * The Channel Labels
   */
  wmove(wLabel,0,1);
  wprintw(wLabel," LEFT:");
  wmove(wLabel,1,1);
  wprintw(wLabel,"RIGHT:");
  wrefresh(wLabel);
  /*
   * The top section
   */
  wmove(wTop,1,27);
  wprintw(wTop,"The Linux Audio Meter");
  wmove(wTop,3,9);
  wprintw(wTop,"      %3d",-dLevel-60);
  wprintw(wTop,"       %3d",-dLevel-50);
  wprintw(wTop,"       %3d",-dLevel-40);
  wprintw(wTop,"       %3d",-dLevel-30);
  wprintw(wTop,"       %3d",-dLevel-20);
  wprintw(wTop,"       %3d",-dLevel-10);
  if(dLevel!=0) {
    wprintw(wTop," ");
  }
  wprintw(wTop,"      %3d",-dLevel);
  wmove(wTop,4,10);
  wprintw(wTop,"      |         |         |         |         |         |         |");
  wrefresh(wTop);
  /*
   * The bottom section
   */
  wmove(wBottom,0,10);
  wprintw(wBottom,"      |         |         |         |         |         |         |");
  wmove(wBottom,1,9);
  wprintw(wBottom,"      %3d",-dLevel-60);
  wprintw(wBottom,"       %3d",-dLevel-50);
  wprintw(wBottom,"       %3d",-dLevel-40);
  wprintw(wBottom,"       %3d",-dLevel-30);
  wprintw(wBottom,"       %3d",-dLevel-20);
  wprintw(wBottom,"       %3d",-dLevel-10);
  if(dLevel!=0) {
    wprintw(wBottom," ");
  }
  wprintw(wBottom,"      %3d",-dLevel);
  wrefresh(wBottom);

}




void UpdateMeter(int dLeft,int dRight,int dRef,int dThreshold)
{
  int i;
  int dColor;

  /*
   * Calculate the color threshold
   */
  dColor=67+20*log10(((double)dThreshold)/((double)dRef));

  /*
   * The left Channel
   */
  if(dLeft==0) {
    dLeft=1;
  }
  dLeft=67+20*log10(((double)dLeft)/((double)dRef));
  if(dLeft<0) {
    dLeft=0;
  }
  if(dLeft>67) {
    dLeft=67;
  }
  werase(wLeft);
  wmove(wLeft,0,0);
  wcolor_set(wLeft,COLOR_GREEN,NULL);
  for(i=0;i<dLeft;i++) {
    if(i==dColor) {
      wcolor_set(wLeft,COLOR_RED,NULL);
    }
    waddch(wLeft,'*');
  }
  /*
   * The right Channel
   */
  if(dLeft==0) {
    dLeft=1;
  }
  dRight=67+20*log10(((double)dRight)/((double)dRef));
  if(dRight<0) {
    dRight=0;
  }
  if(dRight>67) {
    dRight=67;
  }
  werase(wRight);
  wmove(wRight,0,0);
  wcolor_set(wRight,COLOR_GREEN,NULL);
  for(i=0;i<dRight;i++) {
    if(i==dColor) {
      wcolor_set(wRight,COLOR_RED,NULL);
    }
    waddch(wRight,'*');
  }
  wrefresh(wLeft);
  wrefresh(wRight);
}
  


void MeterStop(int dSigno)
{
  endwin();
  exit(0);
}
