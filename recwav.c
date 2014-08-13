/* recwave.c
 *
 *  A utility for recording files in the the Microsoft 'wav' format.
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include "wavlib.h"
#define USAGE "recwave [-v][-d <dev>][-r <rate>][-s <size>][-c <chan>][-t <length>] <wav-file>"
#define DEFAULT_DSP "/dev/dsp"
#define DEFAULT_SAMPLERATE 44100
#define DEFAULT_SAMPLESIZE 16
#define DEFAULT_CHANNELS 2
#define REC_SAMPLES 882000

int dStop=0;
void stoprecord(int);

int main(int argc,char *argv[])
{
  int i,j;
  int hFilename,hAudio;
  struct wavHeader wavHeader;
  int dOptionFound;
  char sDspdev[256];
  unsigned short wChannels=DEFAULT_CHANNELS;
  unsigned dwSamplesPerSec=DEFAULT_SAMPLERATE;
  unsigned short wBitsPerSample=DEFAULT_SAMPLESIZE;
  unsigned dAccum;
  unsigned char aBuffer[AUDIO_BUFFER];
  unsigned dCount=0;
  unsigned dBytes,dSamples;
  unsigned dLength=0;

  /*
   * Set default DSP Device
   */
  strcpy(sDspdev,DEFAULT_DSP);

  /*
   * Parse Options
   */
  if(argc<2) {
    printf(USAGE);
    printf("\n");
    exit(1);
  }
  if(argc==2) {
    if(strcmp(argv[1],"-v")==0) {   /* Version */
      printf("recwav v");
      printf(VERNUM);
      printf("\n");
      exit(0);
    }
  }
  for(i=1;i<argc-1;i++) {
    dOptionFound=0;
    if(strcmp(argv[i],"-d")==0) {    /* Select DSP Device */
      strcpy(sDspdev,argv[++i]);
      dOptionFound=1;
    }
    if(strcmp(argv[i],"-r")==0) {    /* Select Sample Rate */
      if(sscanf(argv[++i],"%u",&dwSamplesPerSec)!=1) {
	printf("recwave: invalid sample rate\n");
	exit(1);
      }
      dOptionFound=1;
    }
    if(strcmp(argv[i],"-t")==0) {    /* Record Time */
      if(sscanf(argv[++i],"%u",&dLength)!=1) {
	printf("recwave: invalid record length\n");
	exit(1);
      }
      dOptionFound=1;
    }
    if(strcmp(argv[i],"-s")==0) {    /* Select Sample Size */
      if(sscanf(argv[++i],"%u",&dAccum)!=1) {
	printf("recwave: invalid sample size\n");
	exit(1);
      }
      if((dAccum!=8)&&(dAccum!=16)) {
	printf("recwave: invalid sample size\n");
	exit(1);
      }
      wBitsPerSample=(unsigned short)dAccum;
      dOptionFound=1;
    }
    if(strcmp(argv[i],"-c")==0) {    /* Select Number of Channels */
      if(sscanf(argv[++i],"%u",&dAccum)!=1) {
	printf("recwave: invalid channel argument\n");
	exit(1);
      }
      if((dAccum!=1)&&(dAccum!=2)) {
	printf("recwave: invalid channel argument\n");
	exit(1);
      }
      wChannels=(unsigned short)dAccum;
      dOptionFound=1;
    }
    if(dOptionFound==0) {
      printf("recwave: invalid option\n");
      exit(1);
    }
  }

  /*
   * If a length is specified, simply pass the appropriate arguments
   * to RecWavFile()
   */
  if(dLength>0) {
    printf("Recording for %u seconds\n",dLength);
    if(RecWavFile(argv[argc-1],sDspdev,dLength,wChannels,dwSamplesPerSec,
		  wBitsPerSample,0)<0) {
      perror("recwav");
      exit(1);
    }
    exit(0);
  }

  /*
   * Open DSP Device
   */
  hAudio=open(sDspdev,O_RDONLY);
  if(hAudio<0) {
    printf("recwave: can't open dsp device\n");
    exit(1);
  }

  /*
   * Open the wav file
   */
  wavHeader.wFormatTag=WAVE_FORMAT_PCM;
  wavHeader.wChannels=wChannels;
  wavHeader.dwSamplesPerSec=dwSamplesPerSec;
  wavHeader.dwAvgBytesPerSec=dwSamplesPerSec*(wBitsPerSample/8)*wChannels;
  wavHeader.wBlockAlign=(wBitsPerSample/8)*wChannels;
  wavHeader.wBitsPerSample=wBitsPerSample;
  hFilename=CreateWav(argv[argc-1],&wavHeader);

  /*
   * Record
   */
  SetDspDesc(hAudio,&wavHeader);
  printf("Recording (CNTL-C to stop)...\n");
  signal(SIGINT,stoprecord);
  while(dStop==0) {
    memset(aBuffer,0,AUDIO_BUFFER);
    j=read(hAudio,aBuffer,AUDIO_BUFFER);
    write(hFilename,aBuffer,j);
    dCount++;
  }
  dBytes=dCount*AUDIO_BUFFER;
  dSamples=dBytes/wavHeader.wBlockAlign;
  FixWav(hFilename,dSamples,dBytes);
  close(hFilename);

  exit(0);
}




void stoprecord(int signo)
{
  dStop++;
}
