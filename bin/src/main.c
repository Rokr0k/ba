#include <ba/writer.h>
#include <getopt.h>
#include <stdlib.h>

static void print_help(const char *arg0) {}

static void print_version(const char *arg0) {}

int main(int argc, char **argv) {
  int ch;
  while ((ch = getopt(argc, argv, "hvo:")) != -1) {
    switch (ch) {
    case 'h':
      print_help(argv[0]);
      exit(0);

    case 'v':
      print_version(argv[0]);
      exit(0);
    }
  }

  return 0;
}
