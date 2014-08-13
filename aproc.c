/* aproc.c
 *
 *  An audio processing utility
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
#define USAGE "aproc [-N <level>][-o <wav-file>] <wav-file>\n"
#define BUFFER_SIZE 8192

int main(int argc,char *argv[])
{
  int i,j;
  int dMatch=0;
  int dOutput=0;
  int dNormalize=0;
  int dNormalLevel=0;
  float dfNormalLevel=0;
  char sInputname[PATH_LEN],sOutputname[PATH_LEN];
  int hInputname,hOutputname=0;
  struct wavHeader wavHeader;
  unsigned char cBuffer[BUFFER_SIZE];
  short *cAudio16;
  int dBuffers=0,dLastBuffer=0;
  int dMax=0,dMin=0;
  int c;

  if(argc<2) {
    printf(USAGE);
    exit(1);
  }

  if(strcmp(argv[argc-1],"-v")==0) {    /* Version */
    printf("aproc v");
    printf(VERNUM);
    printf("\n");
    exit(0);
  }

  /*
   * Get Input Filename
   */
  strcpy(sInputname,argv[argc-1]);

  /* Get options */
  for(i=1;i<argc-1;i++) {
    dMatch=0;
    if(strcmp(argv[i],"-v")==0) {    /* Version */
      printf("aproc v");
      printf(VERNUM);
      printf("\n");
      exit(0);
    }
    if(strcmp(argv[i],"-o")==0) {    /* Output Filename */
      dMatch=1;
      dOutput=1;
      if(sscanf(argv[++i],"%s",sOutputname)!=1) {
	printf("aproc: invalid output filename\n");
	exit(1);
      }
    }
    if(strcmp(argv[i],"-N")==0) {    /* Normalize Function */
      dMatch=1;
      dNormalize=1;
      if(sscanf(argv[++i],"%f",&dfNormalLevel)!=1) {
	printf("aproc: invalid normalization level\n");
	exit(1);
      }
      dNormalLevel=32768*pow(10,-dfNormalLevel/20);
    }
    if(dMatch==0) {
      printf("nmix: invalid option\n");
      exit(1);
    }
  }

  if(dNormalize==1) {   /* Normalize Audio Level */
    hInputname=OpenWav(sInputname,&wavHeader);
    if(hInputname<0) {
      printf("aproc: unable to open input file\n");
      exit(1);
    }
    
    /*
     * Can we handle this format? 
     */
    if(wavHeader.wFormatTag!=WAVE_FORMAT_PCM) {
      printf("aproc: unsupported audio file format\n");
      close(hInputname);
      exit(1);
    }
    if(wavHeader.wBitsPerSample!=16) {
      printf("aproc: unsupported sample size\n");
      close(hInputname);
      exit(1);
    }
    dBuffers=wavHeader.dwWaveDataSize/BUFFER_SIZE;
    dLastBuffer=wavHeader.dwWaveDataSize-(dBuffers*BUFFER_SIZE);
    cAudio16=(short *)cBuffer;
    /*
     * First Pass
     *
     * Determine the current audio normalization level 
     */
    for(i=0;i<dBuffers;i++) {
      c=read(hInputname,cBuffer,BUFFER_SIZE);
      if(c!=BUFFER_SIZE) {
	printf("aproc: unexpected end of audio file\n");
	close(hInputname);
	exit(1);
      }
      for(j=0;j<BUFFER_SIZE/2;j++) {
	if(cAudio16[j]>dMax) {
	  dMax=cAudio16[j];
	}
	if(cAudio16[j]<dMin) {
	  dMin=cAudio16[j];
	}
      }
    }
    c=read(hInputname,cBuffer,dLastBuffer);
    if(c!=dLastBuffer) {
      printf("aproc: unexpected end of audio file\n");
      close(hInputname);
      exit(1);
    }
    for(j=0;j<dLastBuffer/2;j++) {
      if(cAudio16[j]>dMax) {
	dMax=cAudio16[j];
      }
      if(cAudio16[j]<dMin) {
	dMin=cAudio16[j];
      }
    }
    close(hInputname);
    if(abs(dMin)>abs(dMax)) {
      dMax=abs(dMin);
    }
    else {
      dMax=abs(dMax);
    }
    
    /*
     * Second Pass
     *
     * Apply the gain change
     */
    hInputname=OpenWav(sInputname,&wavHeader);
    if(hInputname<0) {
      printf("aproc: unable to open input file\n");
      exit(1);
    }
    if(dOutput==1) {
      hOutputname=CreateWav(sOutputname,&wavHeader);
      if(hOutputname<0) {
	close(hInputname);
	printf("aproc: unable to open output file\n");
	exit(1);
      }
    }
    for(i=0;i<dBuffers;i++) {
      c=read(hInputname,cBuffer,BUFFER_SIZE);
      if(c!=BUFFER_SIZE) {
	printf("aproc: unexpected end of audio file\n");
	close(hInputname);
	if(dOutput==1) {
	  close(hOutputname);
	}
	exit(1);
      }
      for(j=0;j<BUFFER_SIZE/2;j++) {
	cAudio16[j]=(short)((((long)cAudio16[j])*dNormalLevel)/dMax);
      }
      if(dOutput==1) {
	write(hOutputname,cBuffer,BUFFER_SIZE);
      }
      else {
	lseek(hInputname,-BUFFER_SIZE,SEEK_CUR);
	write(hInputname,cBuffer,BUFFER_SIZE);
      }
    }
    c=read(hInputname,cBuffer,dLastBuffer);
    if(c!=dLastBuffer) {
      printf("aproc: unexpected end of audio file\n");
      close(hInputname);
      if(dOutput==1) {
	close(hOutputname);
      }
      exit(1);
    }
    for(j=0;j<dLastBuffer/2;j++) {
      cAudio16[j]=(short)((((long)cAudio16[j])*dNormalLevel)/dMax);
    }
    if(dOutput==1) {
      write(hOutputname,cBuffer,dLastBuffer);
    }
    else {
      lseek(hInputname,-dLastBuffer,SEEK_CUR);
      write(hInputname,cBuffer,dLastBuffer);
    }
    close(hInputname);
    if(dOutput==1) {
      FixWav(hOutputname,wavHeader.dwFileSize,wavHeader.dwWaveDataSize);
      close(hOutputname);
    }
  }
  exit(0);
}
