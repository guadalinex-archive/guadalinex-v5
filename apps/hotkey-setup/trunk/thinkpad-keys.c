/*
 *  Program to generate faked keyboard input using udev / uinput
 *  Copyright (c) 2006 Paul Sladen <ubuntu@paul.sladen.org>
 *
 *  Ideas from 'kbdd' and 'tpb'.
 *  Copyright (C) 2004,2005 Nils Faerber <nils.faerber@kernelconcepts.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <linux/input.h>
#include <linux/uinput.h>

#define _GNU_SOURCE
#include <getopt.h>

#define UINPUT_DEVICE "/dev/input/uinput"
#define NVRAM_DEVICE "/dev/nvram"
#define POLL_DELAY 50 /* milliseconds */
#define UPDATE_FIFO "/var/run/thinkpadkeys.fifo"

static int init_nvram(void)
{
  int fd;

  if((fd = open(NVRAM_DEVICE, O_RDONLY|O_NONBLOCK)) < 0)
    fd = 0;

  return fd;
}

struct record {
  unsigned char thinkpad, zoom, brightness_moved, brightness, volume_moved, muted, volume, thinklight;
};

enum {
  THINKPAD = 1,
  ZOOM = 2,
  VOLUME_UP = 4,
  VOLUME_DOWN = 8,
  VOLUME_MUTE = 16,
  BRIGHTNESS_DOWN = 32,
  BRIGHTNESS_UP = 64,
  THINKLIGHT = 128
};

struct keys {
  unsigned char mask;
  unsigned short keycode;
} codes[] = {
  { THINKPAD, KEY_PROG1 },
  { ZOOM, KEY_F22 }, /* Stand-in for KEY_VIDEOMODECYCLE */
  { VOLUME_DOWN, KEY_VOLUMEDOWN },
  { VOLUME_UP, KEY_VOLUMEUP },
  { VOLUME_MUTE, KEY_MUTE },
  { BRIGHTNESS_DOWN, KEY_BRIGHTNESSDOWN },
  { BRIGHTNESS_UP, KEY_BRIGHTNESSUP },
  { THINKLIGHT, KEY_F19 }, /* Stand-in for KEY_LIGHT */
  { 0, 0 }
};

/* Retrieve the interesting parts of the nvram contents */
static void grok_nvram(int fd, struct record *where)
{
  unsigned char c;
  int thinkpad, zoom, down, up, mute, volume;
  lseek(fd, 0x57, SEEK_SET);
  read(fd, &c, 1);
  
  where->thinkpad = c & 0x08;
  where->zoom = c & 0x20;

  lseek(fd, 0x58, SEEK_SET);
  read(fd, &c, 1);

  where->thinklight = c & 0x10;

  lseek(fd, 0x5e, SEEK_SET);
  read(fd, &c, 1);

  where->brightness_moved = c & 0x20;
  where->brightness = c & 0x07;

  lseek(fd, 0x60, SEEK_SET);
  read(fd, &c, 1);

  where->volume_moved = c & 0xc0;
  where->muted = c & 0x40;
  where->volume = c & 0x0f;
}

static void compare_nvram(int *result, struct record *a, struct record *b)
{
  int r = 0;
  if(a->thinkpad != b->thinkpad)
    r |= THINKPAD;
  if(a->zoom != b->zoom)
    r |= ZOOM;
  if(a->thinklight != b->thinklight)
    r |= THINKLIGHT;

  if (a->brightness_moved != b->brightness_moved) {
      if(a->brightness > b->brightness || !b->brightness)
	  r |= BRIGHTNESS_DOWN;
      else
	  r |= BRIGHTNESS_UP;
  }

  if(a->volume_moved != b->volume_moved)
    if(b->muted)
      {
	r |= VOLUME_MUTE;
	/* This HACK means that pressing MUTE, when already MUTED, stays MUTED */
	if(a->muted)
	  r |= VOLUME_UP;
      }
    else if(a->volume > b->volume || !b->volume)
      r |= VOLUME_DOWN;
    else 
      r |= VOLUME_UP;

  *result = r;
}

static int init_uinput(const char *name, struct keys *table)
{
  int fd, i;
  struct uinput_user_dev dev = {
    .id = {
      .bustype = BUS_I8042,
      .vendor = 0x1014,  /* IBM  */
      .product = 0x5450, /* "TP" */
      .version = 0x0001,
    }
  };

  strncpy(dev.name, name, UINPUT_MAX_NAME_SIZE);

  if((fd = open(UINPUT_DEVICE, O_WRONLY)) < 0)
    {
      perror("Cannot open " UINPUT_DEVICE);
      goto shutdown;
    }

  if(write(fd, &dev, sizeof(dev)) < sizeof(dev))
    {
      perror("Cannot create a uinput device");
      goto release;
    }

  if(ioctl(fd, UI_SET_EVBIT, EV_KEY))
    goto release;

#if 1
  /* Only setup the actual keys we're going to use */
  for(i = 0; table[i].mask; i++)
    if(ioctl(fd, UI_SET_KEYBIT, table[i].keycode))
      goto release;
#else
  /* Setup all possible key codes */
  for(i = 0; i <= KEY_MAX; i++)
    if(ioctl(fd, UI_SET_KEYBIT, i))
      goto release;
#endif

  if(ioctl(fd, UI_DEV_CREATE))
    goto release;

  return fd;

 release:
  ioctl(fd, UI_DEV_DESTROY);
 shutdown:
  close(fd);
  return 0;
}

static int cleanup_uinput_fd = 0;
static __sighandler_t cleanup_uinput_previous = NULL;

void cleanup_uinput(int signal)
{
  if(cleanup_uinput_fd)
    ioctl(cleanup_uinput_fd, UI_DEV_DESTROY);
  if(cleanup_uinput_previous)
    (*cleanup_uinput_previous)(signal);
  else
    exit(EXIT_SUCCESS);
}

static void send_key(int fd, unsigned short code)
{
  struct input_event ev = {
    .type = EV_KEY,
    .code = code,
    .time = {0, }
  };

  if(code > KEY_MAX)
    return;
  
  ev.value = 1; // press...
  write(fd, &ev, sizeof(ev));

  ev.value = 0; // then release
  write(fd, &ev, sizeof(ev));
}

static void punt_keycodes(int fd, int buttons, struct keys *table)
{
  int i;
  for(i = 0; table[i].mask; i++)
    if(buttons & table[i].mask)
      send_key(fd, table[i].keycode);
}

int main(int argc, char **argv)
{
  int nvram, uinput, fifo;
  struct record state[2];
  int buttons, counter = 1;
  int lenovo = 0;
  int mask;
  char command;

  if (argc > 1)
    if (!strcmp(argv[1], "--update") || !strcmp(argv[1], "-u"))
      {
	if((fifo = open (UPDATE_FIFO, O_WRONLY | O_NONBLOCK)) != -1)
	  {
	    write (fifo, "1", sizeof(char));
	    close(fifo);
	  }
	return EXIT_SUCCESS;
      }
    else if (!strcmp(argv[1], "--no-brightness") || !strcmp (argv[1], "LENOVO"))
      lenovo = 1;

#if 1
  /* Software volume control */
  if (lenovo)
    mask = THINKPAD|ZOOM|VOLUME_UP|VOLUME_DOWN|VOLUME_MUTE|THINKLIGHT;
  else
    mask = THINKPAD|ZOOM|VOLUME_UP|VOLUME_DOWN|VOLUME_MUTE|BRIGHTNESS_DOWN|BRIGHTNESS_UP|THINKLIGHT;
#else
  /* Hardware mixer present */
  mask = THINKPAD|ZOOM;
#endif

  if(!(nvram = init_nvram()))
    {
      perror("Could not open nvram device");
      return 1;
    }
  if(!(uinput = init_uinput(*argv, codes)))
    {
      perror("Could not open uinput device");
      close(nvram);
      return 2;
    }

  /* Daemonise into the background */
  if(fork())
    exit(EXIT_SUCCESS);

  /* Clean up nicely */
  cleanup_uinput_previous = signal(SIGTERM, cleanup_uinput);

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  grok_nvram(nvram, state);

  if (mkfifo (UPDATE_FIFO, 0600) < 0) {
	  if (errno != EEXIST) {
		  perror ("Error creating fifo " UPDATE_FIFO);
		  return 1;
	  }
  }

  while ((fifo = open (UPDATE_FIFO, O_RDONLY)) != -1) {
	  read (fifo, &command, sizeof(char));
	  grok_nvram(nvram, state + counter);
	  compare_nvram(&buttons, state + (counter^1), state + counter);
	  if(buttons &= mask)
		  punt_keycodes(uinput, buttons, codes);
	  counter^=1;
	  close (fifo);
  }

  cleanup_uinput(SIGTERM);
}

