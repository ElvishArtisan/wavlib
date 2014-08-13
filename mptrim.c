/* mptrim.c
 *
 *  A utility for triming the start and end of MPEG files.
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
#define USAGE "mptrim [-v][-s <start-offset>][-e <end-offset] <in-file> <out-file>\n"


int main(int argc,char *argv[])
{
  int i;
  int found;
  int start_offset=-1;
  int end_offset=-1;
  char *src_name=NULL;
  char *dest_name=NULL;
  int src_fd;
  int dest_fd;
  unsigned char mpeg_header[4];
  struct wavMpegFmt wavMpegFmt;
  int start_frame;
  int end_frame=0;
  unsigned char *buffer;
  int n;

  /*
   * Parse Options
   */
  if(argc<2) {
    fprintf(stderr,USAGE);
    printf("\n");
    exit(1);
  }
  if(argc==2) {
    if(strcmp(argv[1],"-v")==0) {   /* Version */
      printf("mptrim v");
      printf(VERSION);
      printf("\n");
      exit(0);
    }
    else {
      fprintf(stderr,USAGE);
      exit(1);
    }
  }
  for(i=1;i<argc;i++) {
    found=0;
    if(strcmp(argv[i],"-s")==0) {    /* Start Offset */
      if(++i==(argc-1)) {
	fprintf(stderr,USAGE);
	exit(1);
      }
      if(sscanf(argv[i],"%d",&start_offset)!=1) {
	fprintf(stderr,USAGE);
	exit(1);
      }
      found=1;
    }
    if(strcmp(argv[i],"-e")==0) {    /* End Offset */
      if(++i==(argc-1)) {
	fprintf(stderr,USAGE);
	exit(1);
      }
      if(sscanf(argv[i],"%d",&end_offset)!=1) {
	fprintf(stderr,USAGE);
	exit(1);
      }
      found=1;
    }
    if(found==0) {
      if(i==(argc-2)) {
	src_name=argv[i];
	found=1;
      }
      if(i==(argc-1)) {
	if(src_name==NULL) {
	  fprintf(stderr,USAGE);
	  exit(1);
	}
	dest_name=argv[i];
	found=1;
      }
      if(found==0) {
	fprintf(stderr,USAGE);
	exit(1);
      }
    }
  }

  /*
   * Open Source File
   */
  if((src_fd=open(src_name,O_RDONLY))<0) {
    perror("mptrim");
    exit(1);
  }

  /*
   * Read the first MPEG header
   *
   * FIXME:  This should deal with any ID3 tag found here!
   */
  if(read(src_fd,mpeg_header,4)!=4) {
    close(src_fd);
    fprintf(stderr,"mptrim: source not a valid MPEG file\n");
    exit(1);
  }
  if(GetMpegFormat(mpeg_header,&wavMpegFmt)<0) {
    close(src_fd);
    fprintf(stderr,"mptrim: source not a valid MPEG file\n");
    exit(1);
  }

  /*
   * Calculate Start and End Frame Counts
   */
  if(start_offset<0) {
    start_frame=0;
  }
  else {
    start_frame=start_offset*wavMpegFmt.sample_rate/11520;
  }
  if(end_offset>=0) {
    end_frame=end_offset*wavMpegFmt.sample_rate/11520;
  }

  /*
   * Open Destination File
   */
  if((dest_fd=
      open(dest_name,
	   O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH))<0) {
    perror("mptrim");
    close(src_fd);
    exit(1);
  }

  /*
   * Copy
   */
  buffer=(unsigned char *)malloc(wavMpegFmt.frame_size);
  lseek(src_fd,start_frame*wavMpegFmt.frame_size,SEEK_SET);
  if(end_offset<0) {
    while((n=read(src_fd,buffer,wavMpegFmt.frame_size))>0) {
      write(dest_fd,buffer,n);
    }
  }
  else {
    for(i=start_frame;i<end_frame;i++) {
      n=read(src_fd,buffer,wavMpegFmt.frame_size);
      write(dest_fd,buffer,n);
    }
  }
  free(buffer);

  /*
   * Clean Up
   */
  close(dest_fd);
  close(src_fd);

  exit(0);
}
