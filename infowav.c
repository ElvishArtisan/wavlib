/* infowav.c
 *
 *  A general-purpose utility for analyzing files in the the Microsoft
 *  'wav' format.
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

#include "wavlib.h"
#define USAGE "infowav [-v][-i][-l][-p <dsp-dev>] <wav-file>"


int main(int argc,char *argv[])
{
  int hFilename;
  struct wavHeader wavHeader;
  struct wavChunk wavChunk;
  struct wavList wavList;
  int i;
  int dInfo=0,dList=0,dPlay=0,dOptionFound;
  char sDspdev[256];
  char sFormat[256];


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
      printf("infowav v");
      printf(VERNUM);
      printf("\n");
      exit(0);
    }
    else {
      printf("infowav: invalid argument\n");
      exit(1);
    }
  }
  for(i=1;i<argc-1;i++) {
    dOptionFound=0;
    if(strcmp(argv[i],"-i")==0) {    /* Display General Info */
      dInfo=1;
      dOptionFound=1;
    }
    if(strcmp(argv[i],"-l")==0) {    /* Display Extended List Data */
      dList=1;
      dOptionFound=1;
    }
    if(strcmp(argv[i],"-p")==0) {    /* Play on Specified Device */
      dPlay=1;
      strcpy(sDspdev,argv[++i]);
      if(i==argc-1) {
	printf("infowav: missing wav file argument\n");
	exit(1);
      }
      dOptionFound=1;
    }
    if(dOptionFound==0) {
      printf("infowav: invalid option\n");
      exit(1);
    }
  }

  /*
   * Initialize
   */
  hFilename=OpenWav(argv[argc-1],&wavHeader);
  if(hFilename<0) {
    perror("infowav");
    exit(1);
  }

  /*
   * Display Info
   */
  if(dInfo==1) {
  printf("\n");
  printf("         Wave File: %s\n",argv[argc-1]);
    switch(wavHeader.wFormatTag) {
    case WAVE_FORMAT_PCM:
      strcpy(sFormat,"Linear PCM");
      break;
    case IBM_FORMAT_MULAW:
      strcpy(sFormat,"IBM Mu-Law");
      break;
    case IBM_FORMAT_ALAW:
      strcpy(sFormat,"IBM A-Law");
      break;
    case IBM_FORMAT_ADPCM:
      strcpy(sFormat,"IBM Adaptive-Differential PCM");
      break;
    default:
      strcpy(sFormat,"Unknown");
    }
    printf("     Sample Format: %s (0x%04X)\n",sFormat,(int)wavHeader.wFormatTag);
    printf("          Channels: %d\n",(int)wavHeader.wChannels);
    printf("  Samples/sec/chan: %u\n",wavHeader.dwSamplesPerSec);
    printf("       Bits/sample: %d\n",(int)wavHeader.wBitsPerSample);
    printf("         Bytes/sec: %u\n",wavHeader.dwAvgBytesPerSec);
    printf("   Block Alignment: %d\n",(int)wavHeader.wBlockAlign);
    printf("     Total Samples: %u\n",wavHeader.dwFileSize);
    printf("      Audio Length: %u sec\n",(unsigned)wavHeader.tWavLength);
    close(hFilename);
    printf("       *** CHUNK DATA ***\n");
    printf("Name   --Offset--    ---Size---\n");
    hFilename=open(argv[argc-1],O_RDONLY);
    if(hFilename<0) {
      perror("infowav");
      exit(1);
    }
    lseek(hFilename,12,SEEK_SET);
    while(GetNextChunk(hFilename,&wavChunk)==0) {
      printf("%4s   %10u    %10u\n",wavChunk.sName,(unsigned)wavChunk.oOffset,
	     (unsigned)wavChunk.oSize);
    }
  }

  if(dList==1) {  /* Display extended list data */
    i=GetListChunk(hFilename,&wavList);
    if(i<0) {
      printf("  No extended list attributes found.\n");
    }
    else {
      printf("  *** EXTENDED LIST ATTRIBUTES ***\n");
      if(i==0) {
	printf("ICRD: %s\n",wavList.sIcrd);
      }
      else {
	printf("ICRD: --NOT PRESENT--\n");
      }
      printf("IART: %s\n",wavList.sIart);
      printf("ICMT: %s\n",wavList.sIcmt);
      printf("ICOP: %s\n",wavList.sIcop);
      printf("IENG: %s\n",wavList.sIeng);
      printf("IGNR: %s\n",wavList.sIgnr);
      printf("IKEY: %s\n",wavList.sIkey);
      printf("IMED: %s\n",wavList.sImed);
      printf("INAM: %s\n",wavList.sInam);
      printf("ISFT: %s\n",wavList.sIsft);
      printf("ISRC: %s\n",wavList.sIsrc);
      printf("ITCH: %s\n",wavList.sItch);
      printf("ISBJ: %s\n",wavList.sIsbj);
      printf("ISRF: %s\n",wavList.sIsrf);
    }
  }

  close(hFilename);

  if(dPlay==1) {     /* Play the wavfile */
    printf("\n");
    printf("Playing: %s\n",argv[argc-1]);
    if(PlayWavFile(argv[argc-1],sDspdev,0)<0) {
      perror("infowav");
      exit(1);
    }
  }


  exit(0);
}
