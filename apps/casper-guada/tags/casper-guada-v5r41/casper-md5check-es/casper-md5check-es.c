/* casper-md5check - a tool to check md5sums and talk to usplash
   (C) Canonical Ltd 2006
   Written by Tollef Fog Heen <tfheen@ubuntu.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA. */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include <termios.h>

#define USPLASH_FIFO "/dev/.initramfs/usplash_fifo"
#define MAXTRIES 5
#include "md5.h"
#define DEBUG

static int verbose = 1;
static int got_usplash = 0;

void parse_cmdline(void) {
  FILE *cmdline = fopen("/proc/cmdline", "r");
  char buf[1024];
  size_t len;
  char *bufp = buf, *tok;

  if (!cmdline)
    return;

  len = fread(buf, 1023, 1, cmdline);
  buf[len] = '\0';

  while ((tok = strsep(&bufp, " ")) != NULL) {
    if (strncmp(tok, "quiet", 5) == 0)
      verbose = 0;
  }

  fclose(cmdline);
}

int write_and_retry(int fd, char *s) {
  int try = 0, ret = 0;
  char *end;

#ifdef DEBUG
  fprintf(stderr, "-> %s\n", s);
#endif

  end = s + strlen(s)+1;

  ret = write(fd, s, end - s);
  while (s + ret < end && try < MAXTRIES) {
    sleep(1);
    s += ret;
    ret = write(fd, s, strlen(s)+1);
    try++;
  }
  return (s+ret == end ? 0 : 1);
}

void usplash_timeout(int fd, int timeout) {
  char *s;

  if (!got_usplash)
    return;

  asprintf(&s, "TIMEOUT %d", timeout);

  write_and_retry(fd, s);

  free(s);
}

void usplash_failure(int fd, char *format, ...) {
  char *s, *s1;
  va_list argp;

  va_start(argp, format);
  vasprintf(&s, format, argp);
  va_end(argp);

  if (got_usplash) {
    asprintf(&s1, "FAILURE %s", s);

    write_and_retry(fd, s1);

    free(s1);
  } else if (verbose)
    printf("%s\n", s);

  free(s);
}

void usplash_text(int fd, char *format, ...) {
  char *s, *s1;
  va_list argp;

  va_start(argp, format);
  vasprintf(&s, format, argp);
  va_end(argp);

  if (got_usplash) {
    asprintf(&s1, "TEXT %s", s);

    write_and_retry(fd, s1);

    free(s1);
  } else if (verbose) {
    printf("%s...", s);
    fflush(stdout);
  }

  free(s);
}

void usplash_urgent(int fd, char *format, ...) {
  char *s, *s1;
  va_list argp;

  va_start(argp, format);
  vasprintf(&s, format, argp);
  va_end(argp);

  if (got_usplash) {
    asprintf(&s1, "TEXT-URGENT %s", s);

    write_and_retry(fd, s1);

    free(s1);
  } else
    printf("%s\n", s);

  free(s);
}


void usplash_success(int fd, char *format, ...) {
  char *s, *s1;
  va_list argp;

  va_start(argp, format);
  vasprintf(&s, format, argp);
  va_end(argp);

  if (got_usplash) {
    asprintf(&s1, "SUCCESS %s", s);

    write_and_retry(fd, s1);
    
    free(s1);
  } else if (verbose)
    printf("%s\n", s);

  free(s);
}

void usplash_progress(int fd, int progress) {
  static int prevprogress = -1;
  char *s;

  if (progress == prevprogress)
    return;
  prevprogress = progress;

  if (got_usplash) {
    asprintf(&s, "PROGRESS %d", progress);

    write_and_retry(fd, s);

    free(s);
  } else {
    printf("%d%%...", progress);
    fflush(stdout);
  }
}

int set_nocanonical_tty(int fd) {
  struct termios t;

  if (tcgetattr(fd, &t) == -1) {
    perror("tcgetattr");
  }
  t.c_lflag &= ~ICANON;
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;
  return tcsetattr(fd, TCSANOW, &t);
}

int main(int argc, char **argv) {
  
  int pipe_fd, check_fd;
  int failed = 0;
  
  FILE *md5_file;
  md5_state_t state;
  md5_byte_t digest[16];
  char hex_output[16*2 + 1];
  char *checksum, *checkfile;
  ssize_t tsize, csize;

  tsize = 0;
  csize = 0;

  if (argc != 3) {
    // fprintf(stderr,"Wrong number of arguments\n");
    fprintf(stderr,"Numero de argumentos erroneo\n");
    //fprintf(stderr,"%s <root directory> <md5sum file>\n", argv[0]);
    fprintf(stderr,"%s <directorio raiz> <archivo md5sum>\n", argv[0]);
    exit(1);
  }
  
  if (chdir(argv[1]) != 0) {
    perror("chdir");
    exit(1);
  }
  
  parse_cmdline();

  pipe_fd = open(USPLASH_FIFO, O_WRONLY|O_NONBLOCK);
  
  if (pipe_fd == -1) {
    /* Fall back to text output */
    perror("Opening pipe");
    got_usplash = 0;
  } else
    got_usplash = 1;
  

  usplash_progress(pipe_fd, 0);
  // usplash_urgent(pipe_fd, "Checking integrity, this may take some time");
  usplash_urgent(pipe_fd, "Comprobando la integridad del CD. Esto puede tardar.");
  md5_file = fopen(argv[2], "r");
  if (!md5_file) {
          perror("fopen md5_file");
          exit(1);
  }
  while (fscanf(md5_file, "%as %as", &checksum, &checkfile) == 2) {
    struct stat statbuf;

    if (stat(checkfile, &statbuf) == 0) {
      tsize += statbuf.st_size;
    }

    free(checksum);
    free(checkfile);
  }

  rewind(md5_file);
  while (fscanf(md5_file, "%as %as", &checksum, &checkfile) == 2) {
    char buf[BUFSIZ];
    ssize_t rsize;
    int i;
    
    md5_init(&state);
    
    // usplash_text(pipe_fd, "Checking %s", checkfile);
    usplash_text(pipe_fd, "Comprobando %s", checkfile);
    
    check_fd = open(checkfile, O_RDONLY);
    if (check_fd < 0) {
      usplash_timeout(pipe_fd, 300);
      usplash_failure(pipe_fd, "%s", strerror(errno));
      sleep(10);
    }
    
    rsize = read(check_fd, buf, sizeof(buf));

    while (rsize > 0) {
      csize += rsize;
      usplash_progress(pipe_fd, floorl(100*((long double)csize)/tsize));

      md5_append(&state, (const md5_byte_t *)buf, rsize);
      rsize = read(check_fd, buf, sizeof(buf));
    }
    
    close(check_fd);
    md5_finish(&state, digest);
    
    for (i = 0; i < 16; i++)
      sprintf(hex_output + i * 2, "%02x", digest[i]);
    
    if (strncmp(hex_output, checksum, strlen(hex_output)) == 0) {
      usplash_success(pipe_fd, "OK");
    } else {
      // usplash_failure(pipe_fd, "mismatch");
      usplash_failure(pipe_fd, "no coincide");
      failed++;
    }
    free(checksum);
    free(checkfile);
  }
  fclose(md5_file);
  if (failed) {
    // usplash_urgent(pipe_fd, "Check finished: errors found in %d files!", failed);
    usplash_urgent(pipe_fd, "Comprobacion terminada. Se encontraron fallos en %d ficheros.", failed);
  } else {
    // usplash_urgent(pipe_fd, "Check finished: no errors found");
    usplash_urgent(pipe_fd, "Comprobacion finalizada, no se encontraron errores.");
  }
  // usplash_urgent(pipe_fd, "Press any key to reboot your system");
  usplash_urgent(pipe_fd, "Pulse cualquier tecla para reiniciar su sistema");
  usplash_timeout(pipe_fd, 86400);
  set_nocanonical_tty(0);
  getchar();
  reboot(LINUX_REBOOT_CMD_RESTART);
  return 0;
  
}
