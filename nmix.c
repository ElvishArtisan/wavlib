/* nmix.c
 *
 *  An OSS-compliant mixer utility
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
#include <string.h>

#include "wavlib.h"

#define PATH_LEN 256
#define USAGE "nmix [-e][-rs|-rc <rec-dev>][-s <control> <left>:<right>] <device>\n"

/*
 * Global Variables
 */
char sChannels[SOUND_MIXER_NRDEVICES][8]=SOUND_DEVICE_NAMES;
char sLabels[SOUND_MIXER_NRDEVICES][8]=SOUND_DEVICE_LABELS;

/*
 * Function Prototypes
 */
int GetControl(char *);


int main(int argc,char *argv[])
{
  int i;
  int hAudio;
  int dValue=-1;
  char sControl[PATH_LEN];
  int dControl=0;
  int dCaps,dLevel,dChans,dRecord,dStereo,dSource,dRequest,dOutmask,dOutsrc;
  int dLeft,dRight;
  int dMatch=0;
  char sDevice[PATH_LEN];  /* Name of audio device */
  int dEnum=1,dSet=0,dRec=0;

  if(argc<2) {
    printf(USAGE);
    exit(0);
  }

  if(argc==2) {
    if(strcmp(argv[1],"-v")==0) {
      printf("nmix v");
      printf(VERNUM);
      printf("\n");
      exit(0);
    }
  }

  /* Get device name */
  if(sscanf(argv[argc-1],"%s",sDevice)!=1) {
    printf("nmix: invalid mixer device\n");
    exit(1);
  }
  hAudio=open(sDevice,O_RDWR);
  if(hAudio<0) {
    printf("nmix: cannot open mixer device\n");
    exit(1);
  }

  /* Get options */
  for(i=1;i<argc-1;i++) {
    dMatch=0;
    if(strcmp(argv[i],"-e")==0) {     /* Enumerate the Mixer */
      dMatch=1;
      dEnum=1;
    }
    if(strcmp(argv[i],"-s")==0) {    /* Set Control Value */
      dMatch=1;
      dSet=1;
      if(sscanf(argv[++i],"%s",sControl)!=1) {
	printf("nmix: invalid control\n");
	exit(1);
      }
      dControl=GetControl(sControl);
      if(dControl<0) {
	printf("nmix: unknown control\n");
	exit(1);
      }
      dValue=sscanf(argv[++i],"%d:%d",&dLeft,&dRight);
      if(dValue==0) {
	printf("nmix: invalid value\n");
	exit(1);
      }
      if(dValue==1) {
	dRight=dLeft;
      }
      if((dLeft<0)||(dLeft>100)) {
	printf("nmix: invalid value\n");
	exit(1);
      }
      if((dRight<0)||(dRight>100)) {
	printf("nmix: invalid value\n");
	exit(1);
      }
      dValue=(dLeft<<8)|(dRight&0xff);
    }
    if(strcmp(argv[i],"-rs")==0) {    /* Set Record Device */
      dMatch=1;
      dRec=1;
      if(sscanf(argv[++i],"%s",sControl)!=1) {
	printf("nmix: invalid control\n");
	exit(1);
      }
      dControl=GetControl(sControl);
      if(dControl<0) {
	printf("nmix: unknown control\n");
	exit(1);
      }
    }
    if(strcmp(argv[i],"-rc")==0) {    /* Clear Record Device */
      dMatch=1;
      dRec=2;
      if(sscanf(argv[++i],"%s",sControl)!=1) {
	printf("nmix: invalid control\n");
	exit(1);
      }
      dControl=GetControl(sControl);
      if(dControl<0) {
	printf("nmix: unknown control\n");
	exit(1);
      }
    }
    if(dMatch==0) {
      printf("nmix: invalid option\n");
      exit(1);
    }
  }

  /*
   * Set a mixer control
   */
  if(dSet==1) {
    if(ioctl(hAudio,SOUND_MIXER_READ_DEVMASK,&dChans)<0) {
      printf("nmix: mixer not found\n");
      close(hAudio);
      exit(1);
    }
    if((dChans&(1<<dControl))!=0) {
      if(ioctl(hAudio,MIXER_WRITE(dControl),&dValue)<0) {
	printf("nmix: control set failed\n");
	close(hAudio);
	exit(1);
      }
      else {
	if(ioctl(hAudio,SOUND_MIXER_READ_STEREODEVS,&dStereo)<0) {
	  printf("nmix: control set failed\n");
	  close(hAudio);
	  exit(1);
	}
	if((dStereo&(1<<dControl))!=0) {
	  printf("nmix: %s set to %d:%d\n",sChannels[dControl],dLeft,dRight);
	}
	else {
	  printf("nmix: %s set to %d\n",sChannels[dControl],dLeft);
	}
	close(hAudio);
	exit(0);
      }
    }
  }

  /*
   * Set a recording device
   */
  if(dRec==2) {
    if(ioctl(hAudio,SOUND_MIXER_READ_RECMASK,&dRecord)<0) {
      printf("nmix: mixer not found\n");
      close(hAudio);
      exit(1);
    }
    if((dRecord&(1<<dControl))!=0) {
      ioctl(hAudio,SOUND_MIXER_READ_RECSRC,&dRequest);
      dRequest&=(~(1<<dControl));
      if(ioctl(hAudio,SOUND_MIXER_WRITE_RECSRC,&dRequest)<0) {
	printf("nmix: command set failed\n");
	close(hAudio);
	exit(1);
      }
      close(hAudio);
      exit(0);
    }
    else {
      printf("nmix: recording not supported on this control\n");
      close(hAudio);
      exit(1);
    }
  }

  /*
   * Clear a recording device
   */
  if(dRec==1) {
    if(ioctl(hAudio,SOUND_MIXER_READ_RECMASK,&dRecord)<0) {
      printf("nmix: mixer not found\n");
      close(hAudio);
      exit(1);
    }
    if((dRecord&(1<<dControl))!=0) {
      ioctl(hAudio,SOUND_MIXER_READ_RECSRC,&dRequest);
      dRequest|=(1<<dControl);
      if(ioctl(hAudio,SOUND_MIXER_WRITE_RECSRC,&dRequest)<0) {
	printf("nmix: command set failed\n");
	close(hAudio);
	exit(1);
      }
      close(hAudio);
      exit(0);
    }
    else {
      printf("nmix: recording not supported on this control\n");
      close(hAudio);
      exit(1);
    }
  }


  /*
   * Enumerate the mixer controls
   */
  if(dEnum==1) {
    printf("\n");
    if(ioctl(hAudio,SOUND_MIXER_READ_DEVMASK,&dChans)<0) {
      printf("nmix: mixer not found\n");
      close(hAudio);
      exit(1);
    }
    else {
      ioctl(hAudio,SOUND_MIXER_READ_RECMASK,&dRecord);
      ioctl(hAudio,SOUND_MIXER_READ_STEREODEVS,&dStereo);
      ioctl(hAudio,SOUND_MIXER_READ_RECSRC,&dSource);
      ioctl(hAudio,SOUND_MIXER_OUTMASK,&dOutmask);
      ioctl(hAudio,SOUND_MIXER_OUTSRC,&dOutsrc);
      ioctl(hAudio,SOUND_MIXER_READ_CAPS,&dCaps);
      printf("|-------------------------------------------------------------|\n");
      printf("| DEVICE: %11s                                         |\n",sDevice);
      printf("|-------------------------------------------------------------|\n");
      printf("|     LABEL     | CHANNEL |  LEVEL  | RECORD  | REC | STEREO  |\n");
      printf("|               |         |  (L:R)  | CAPABLE | SRC | CONTROL |\n");
      printf("|---------------|---------|---------|---------|-----|---------|\n");
      for(i=0;i<SOUND_MIXER_NRDEVICES;i++) {
	if((dChans&(1<<i))!=0) {
	  if(ioctl(hAudio,MIXER_READ(i),&dLevel)<0) {
	    dLevel=-1;
	  }
	  dLeft=dLevel>>8;
	  dRight=dLevel&0xff;
	  if((dStereo&(1<<i))!=0) {
	    printf("| %-13s | %-7s | %3d:%3d |   ",sLabels[i],sChannels[i],
		   dLeft,dRight);
	  }
	  else {
	    printf("| %-13s | %-7s | %3d: -- |   ",sLabels[i],sChannels[i],
		   dLeft);
	  }
	  if((dRecord&(1<<i))!=0) {
	    printf("yes   |  ");
	  }
	  else {
	    printf("no    |  ");
	  }
	  if((dSource&(1<<i))!=0) {
	    printf("*  |   ");
	  }
	  else {
	    printf("   |   ");
	  }
	  if((dStereo&(1<<i))!=0) {
	    printf("yes   |");
	  }
	  else {
	    printf("no    |");
	  }
	  printf("\n");
	}
      }
      printf("|-------------------------------------------------------------|\n");
      if((dCaps&SOUND_CAP_EXCL_INPUT)!=0) {
	printf("| Only one record source may be assigned at a time.           |\n");
      }
      else {
	printf("| Multiple record sources may be assigned                     |\n"); 
	printf("| simultaneously.                                             |\n");
      }
      printf("|-------------------------------------------------------------|\n");
    }
    printf("\n");
  }

  exit(0);
}



int GetControl(char *sControl)
{
  int i;

  for(i=0;i<SOUND_MIXER_NRDEVICES;i++) {
    if(strcasecmp(sControl,sChannels[i])==0) {
      return i;
    }
  }

  return -1;
}
