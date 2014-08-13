/* agen.c
 *
 *  This plays a tone on the specified device
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

#define BUFFER_SIZE 16384
#define SAMPLE_RATE 44100
#define SAMPLE_SIZE 16
#define USAGE "agen [-f freq][-l level][-c left|right|both][-p now|rev][-t secs][-v] <device>\n"

int main(int argc,char *argv[])
{
  int i;
  int hAudio;
  unsigned char cBuffer[BUFFER_SIZE];
  int dRequest;
  float fFreq=1000;
  float fLevel=0;
  float fRatio;
  int dChan=0;     /* 0=Both, 1=Left, 2=Right */
  char sChan[10];
  int dPhase=0;    /* 0=Normal, 1=Reverse */
  char sPhase[10];
  int dTime=0;
  int dMatch=0;
  int dDebug=0;
  int dSampleRate;
  char sDevice[128];  /* Name of audio device */
  float fSampleRate;
  float fGain;
  float fAngle;
  unsigned long ldCount,ldTimeline=0,ldLimit=BUFFER_SIZE;
  short int dSample;
  time_t tmTimestamp,tmTime;

  if(argc<2) {
    printf(USAGE);
    exit(0);
  }

  if(argc==2) {
    if(strcmp(argv[1],"-v")==0) {
      printf("agen v");
      printf(VERNUM);
      printf("\n");
      exit(0);
    }
  }

  /* Get device name */
  if(sscanf(argv[argc-1],"%s",sDevice)!=1) {
    printf("agen: invalid audio device\n");
    exit(1);
  }

  /* Get options */
  for(i=1;i<argc-1;i++) {
    dMatch=0;
    if(strcmp(argv[i],"-v")==0) {     /* Version */
      printf("agen v");
      printf(VERNUM);
      printf("\n");
      exit(0);
    }
    if(strcmp(argv[i],"-d")==0) {     /* Debug Mode */
      dMatch=1;
      dDebug=1;
    }
    
    if(strcmp(argv[i],"-f")==0) {     /* Frequency */
      dMatch=1;
      if(sscanf(argv[++i],"%f",&fFreq)!=1) {
	printf("agen: invalid frequency\n");
	exit(1);
      }
    }
    if(strcmp(argv[i],"-l")==0) {    /* Level */
      dMatch=1;
      if(sscanf(argv[++i],"%f",&fLevel)!=1) {
	printf("agen: invalid level\n");
	exit(1);
      }
      fLevel=-fLevel;
    }
    if(strcmp(argv[i],"-t")==0) {    /* Time */
      dMatch=1;
      if(sscanf(argv[++i],"%d",&dTime)!=1) {
	printf("agen: invalid time interval\n");
	exit(1);
      }
    }
    if(strcmp(argv[i],"-c")==0) {    /* Channel */
      dMatch=1;
      if(sscanf(argv[++i],"%s",sChan)!=1) {
	printf("agen: invalid time interval\n");
	exit(1);
      }
      if(strcasecmp(sChan,"both")==0) {
	dChan=0;
      }
      else {
	if(strcasecmp(sChan,"left")==0) {
	  dChan=1;
	}
	else {
	  if(strcasecmp(sChan,"right")==0) {
	    dChan=2;
	  }
	  else {
	    printf("agen: invalid channel\n");
	    exit(1);
	  }
	}
      }  
    }
    if(strcmp(argv[i],"-p")==0) {    /* Phase */
      dMatch=1;
      if(sscanf(argv[++i],"%s",sPhase)!=1) {
	printf("agen: invalid phase setting\n");
	exit(1);
      }
      if(strcasecmp(sPhase,"norm")==0) {
	dPhase=0;
      }
      else {
	if(strcasecmp(sPhase,"rev")==0) {
	  dPhase=1;
	}
	else {
	  printf("agen: invalid phase setting\n");
	  exit(1);
	}
      }  
    }
    if(dMatch==0) {
      printf("agen: invalid option\n");
      exit(1);
    }
  }

  /* Convert db to ratio */
  fRatio=pow(10,(fLevel/20));

  /* Open the audio device */
  hAudio=open(sDevice,O_WRONLY);
  if(hAudio<0) {
    printf("agen: can't open audio device\n");
    exit(1);
  }

  /* Set channelization */
  dRequest=1;     /* Stereo Mode */
  ioctl(hAudio,SNDCTL_DSP_STEREO,&dRequest);
  if(dRequest!=1) {    
    printf("agen: can't configure channelization on %s\n",sDevice);
    close(hAudio);
    exit(1);
  }

  /* Set sample size */
  switch(SAMPLE_SIZE) {
  case 8:
    dRequest=AFMT_U8;
    ioctl(hAudio,SNDCTL_DSP_SETFMT,&dRequest);
    if(dRequest!=AFMT_U8) {
      printf("agen: eight bit samples not supported on %s\n",sDevice);
      close(hAudio);
      exit(1);
    }
    break;
  case 16:
    dRequest=AFMT_S16_LE;
    ioctl(hAudio,SNDCTL_DSP_SETFMT,&dRequest);
    if(dRequest!=AFMT_S16_LE) {
      printf("agen: sixteen bit samples not supported on %s\n",sDevice);
      close(hAudio);
      exit(1);
    }
    break;
  }

  /* Determine Effective Sampling Rate */
  dSampleRate=SAMPLE_RATE;
  ioctl(hAudio,SNDCTL_DSP_SPEED,&dSampleRate);
  fSampleRate=(float)dSampleRate;

  /* Display Settings (if requested) */
  if(dDebug==1) {
    printf("--Audio Generator Settings--\n");
    printf("Frequency: %5.0f Hz\n",fFreq);
    printf("Level: %3.1f dB\n",fLevel);
    printf("Channel(s): ");
    switch(dChan) {
    case 0:
      printf("BOTH\n");
      break;
    case 1:
      printf("LEFT\n");
      break;
    case 2:
      printf("RIGHT\n");
      break;
    }
    printf("Phasing: ");
    switch(dPhase) {
    case 0:
      printf("NORMAL\n");
      break;
    case 1:
      printf("REVERSE\n");
      break;
    }
    printf("Effective Sample Rate: %d samples/sec\n",dSampleRate);
  }

  /* Setup time data */
  time(&tmTimestamp);
  tmTime=tmTimestamp;
  if(dTime>0) {
    tmTimestamp+=dTime;
  }
  else {
    tmTimestamp--;
  }

  /* Output audio */

  switch(SAMPLE_SIZE) {
  case 8:
    fGain=127*fRatio;
    ldTimeline=0;
    ldLimit=BUFFER_SIZE/2;
    while(tmTimestamp!=tmTime) { 
      i=0;
      for(ldCount=ldTimeline;ldCount<ldLimit;ldCount++) {
	fAngle=2*PI*ldCount*fFreq/fSampleRate;
	if(dChan==0 || dChan==1) {
	  cBuffer[i++]=fGain*sin(fAngle)+128;
	}
	else {
	  cBuffer[i++]=128;
	}
	if(dChan==0 || dChan==2) {
	  if(dPhase==0) {
	    cBuffer[i++]=fGain*sin(fAngle)+128;
	  }
	  else {
	    cBuffer[i++]=-fGain*sin(fAngle)+128;
	  }
	}
	else {
	  cBuffer[i++]=128;
	}
      }
      write(hAudio,cBuffer,BUFFER_SIZE);
      ldLimit+=(BUFFER_SIZE/2);
      ldTimeline+=(BUFFER_SIZE/2);
      time(&tmTime);
    }  
    break;
  case 16:
    fGain=32767*fRatio;
    ldTimeline=0;
    ldLimit=BUFFER_SIZE/4;
    while(tmTimestamp!=tmTime) { 
      i=0;
      for(ldCount=ldTimeline;ldCount<ldLimit;ldCount++) {
	fAngle=2*PI*ldCount*fFreq/fSampleRate;
	dSample=fGain*sin(fAngle);
	if(dChan==0 || dChan==1) {
	  cBuffer[i++]=0xFF&dSample;
	  cBuffer[i++]=(0xFF&(dSample>>8));  
	}
	else {
	  cBuffer[i++]=0;
	  cBuffer[i++]=0;
	}
	if(dChan==0 || dChan==2) {
	  if(dPhase==1) {
	    dSample=-dSample;
	  }
	  cBuffer[i++]=0xFF&dSample;
	  cBuffer[i++]=(0xFF&(dSample>>8));
	} 
	else {
	  cBuffer[i++]=0;
	  cBuffer[i++]=0;
	}
      }
      write(hAudio,cBuffer,BUFFER_SIZE);
      ldLimit+=(BUFFER_SIZE/4);
      ldTimeline+=(BUFFER_SIZE/4);
      time(&tmTime);
    }  
  break;
  }

  /* close files and finish */
  close(hAudio);

  exit(0);
}
