/* trim.c
 *
 *  An audio trimming utility
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
#define USAGE "trim [-e][-t <threshold>] <wav-file>\n"
#define BUFFER_SIZE 8192

int main(int argc,char *argv[])
{
  int i;
  int dMatch=0;
  int dOutput=0;
  int dTrim=0;
  float fThreshold=0;
  int dThreshold=0;
  char sInputname[PATH_LEN],sOutputname[PATH_LEN];

  if(argc<2) {
    printf(USAGE);
    exit(1);
  }

  if(strcmp(argv[argc-1],"-v")==0) {    /* Version */
    printf("trim v");
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
      printf("trim v");
      printf(VERNUM);
      printf("\n");
      exit(0);
    }
    if(strcmp(argv[i],"-o")==0) {    /* Output Filename */
      dMatch=1;
      dOutput=1;
      if(sscanf(argv[++i],"%s",sOutputname)!=1) {
	printf("trim: invalid output filename\n");
	exit(1);
      }
    }
    if(strcmp(argv[i],"-e")==0) {    /* End Trim Function */
      dMatch=1;
      dTrim=1;
    }
    if(strcmp(argv[i],"-t")==0) {    /* Threshold Value */
      dMatch=1;
      if(sscanf(argv[++i],"%f",&fThreshold)!=1) {
	printf("trim: invalid threshold value\n");
	exit(1);
      }
      dThreshold=(int)(pow(10,-fThreshold/20)*32768);
    }
    if(dMatch==0) {
      printf("trim: invalid option\n");
      exit(1);
    }
  }
  if(dTrim==0) {
    printf("trim: no command specified\n");
    exit(1);
  }
  if(dThreshold==0) {
    printf("trim: no threshold specified\n");
    exit(1);
  }
  if(TailTrim(sInputname,dThreshold)<0) {
    perror("trim");
    exit(1);
  }

  exit(0);
}
