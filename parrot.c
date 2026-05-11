/* Enable glibc bsd support for arc4random */
#define _DEFAULT_SOURCE

#include <ctype.h>
#include <getopt.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Pull in external getopt globals */
extern char* optarg;
extern int optind;
extern int optopt;
extern int opterr;

static const char* USAGE = "Usage: parrot [OPTION]...\n"
"Repeats whatever you tell it.\n"
"\n"
"   -w  Set width of the text balloon\n"
"   -v  Print version and exit\n"
"   -h  Show this message and exit\n";

static const int DEFAULT_WIDTH = 40;
static const int TABSHIFT = 8;
static const float GOLDENISH_RATIO = 1.5;

static const char SURROUNDS[][2] = {
  {'|', '|'},
  {'<', '>'},
  {'/', '\\'},
  {'\\', '/'}
};

static const int PADDING = 2;
static const int MAX_COLOUR_CODE = 255;
static const int GREY_ZERO = 16;
static const int GREY_THREE = 232;

static const char* PARROT =
"       \\\x1b[49m\n"
"        \\     \x1b[38;5;16m▄▄▄\x1b[38;5;232m▄\x1b[38;5;16m▄▄▄▄\x1b[39m\n"
"         \\  \x1b[38;5;16m▄\x1b[48;5;16m \x1b[38;5;Fm▄\x1b[48;5;Cm      \x1b[48;5;232m▄\x1b[48;5;16m▄ \x1b[49m\x1b[38;5;16m▄\n"
"          ▄\x1b[48;5;16m \x1b[38;5;Fm▄\x1b[48;5;Cm           \x1b[48;5;16m \x1b[49m\x1b[38;5;16m▄\n"
"         \x1b[48;5;16m \x1b[38;5;Fm▄\x1b[48;5;Cm   \x1b[48;5;16m \x1b[48;5;Cm  \x1b[38;5;232m▄\x1b[48;5;16m\x1b[38;5;132m▄▄▄\x1b[48;5;Cm\x1b[38;5;16m▄ \x1b[48;5;16m \x1b[48;5;Cm \x1b[48;5;16m \x1b[49m\n"
"        \x1b[48;5;16m \x1b[38;5;Fm▄\x1b[48;5;Cm      \x1b[48;5;16m \x1b[48;5;132m     \x1b[48;5;16m \x1b[48;5;Cm  \x1b[48;5;16m▄ \x1b[49m\x1b[38;5;16m▄\n"
"       \x1b[48;5;16m \x1b[38;5;Fm▄\x1b[48;5;Cm       \x1b[48;5;16m \x1b[48;5;132m     \x1b[48;5;16m \x1b[48;5;Cm    \x1b[48;5;16m \x1b[49m\n"
"       \x1b[48;5;16m \x1b[48;5;Cm        \x1b[48;5;16m \x1b[48;5;132m\x1b[38;5;233m▄    \x1b[48;5;16m \x1b[48;5;Cm    \x1b[48;5;16m \x1b[49m\n"
"       \x1b[48;5;16m \x1b[48;5;Cm         \x1b[48;5;16m \x1b[48;5;132m\x1b[38;5;232m▄ \x1b[38;5;16m▄\x1b[48;5;16m \x1b[48;5;Cm     \x1b[48;5;16m \x1b[49m\n"
"       \x1b[48;5;16m  \x1b[48;5;Cm▄        \x1b[48;5;16m\x1b[38;5;Fm▄ ▄\x1b[48;5;Cm      \x1b[48;5;16m▄▄ \x1b[49m\x1b[38;5;16m▄\n"
"        \x1b[48;5;16m  \x1b[48;5;Cm▄                   \x1b[48;5;233m\x1b[38;5;Fm▄\x1b[48;5;16m▄ \x1b[49m\x1b[38;5;16m▄\n"
"        \x1b[48;5;16m \x1b[48;5;Cm \x1b[48;5;16m\x1b[38;5;Fm▄ \x1b[48;5;Cm\x1b[38;5;16m▄▄▄▄                 \x1b[48;5;16m\x1b[38;5;Fm▄▄ \x1b[49m\x1b[38;5;16m▄\n"
"        \x1b[48;5;16m \x1b[48;5;Cm     \x1b[48;5;16m\x1b[38;5;Fm▄▄▄▄\x1b[48;5;Cm                  \x1b[48;5;232m▄\x1b[48;5;16m▄ \x1b[49m\n"
"        \x1b[38;5;16m▀\x1b[38;5;232m▀▀▀\x1b[38;5;16m▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀\x1b[39m\n";

static inline void null_assert(void* block)
{
  if (block == NULL) {
    perror("Failed to allocate memory");
    exit(EXIT_FAILURE);
  }
}

static inline void* allocate(size_t size)
{
  void* tmp = malloc(size);
  null_assert(tmp);
  return tmp;
}

static inline void* resize(void* ptr, size_t size)
{
  void* tmp = realloc(ptr, size);
  null_assert(tmp);
  return tmp;
}

static inline uint32_t rand_int(uint32_t max_int)
{
  return arc4random_uniform(max_int) + 1;
}

static uint32_t u8strlen(const char* s)
{
  uint32_t len = 0;

  while (*s) {
    len += ((*s++ & 0xC0) != 0x80);
  }

  return len;
}

static void add_line(char** lines, char* line, uint32_t index, uint32_t* max)
{
  lines[index] = line;

  uint32_t len = u8strlen(line);

  if (len > *max) {
    *max = len;
  }
}

static char** wrap_text(char* str, uint32_t width, uint32_t* restrict line_count, uint32_t* restrict max_line)
{
  char* last_space = NULL;
  char* line_start = str;
  char* p;

  size_t lines_size = 10;
  char** lines = allocate(lines_size * sizeof(char*));

  for (p = str; *p; p++) {
    if (*p == ' ') {
      last_space = p;
    }

    bool is_newline = *p == '\n';
    if (is_newline || (p - line_start > width && last_space)) {
      char* line_end = is_newline ? p : last_space;
      *line_end = '\0';
      add_line(lines, line_start, (*line_count)++, max_line);
      line_start = line_end + 1;
      last_space = NULL;
    }

    if (*line_count == lines_size) {
      lines_size *= GOLDENISH_RATIO;
      lines = resize(lines, lines_size * sizeof(char*));
    }
  }

  if (p > line_start) {
    add_line(lines, line_start, (*line_count)++, max_line);
  }

  return lines;
}

static uint32_t get_colour()
{
  uint32_t c = 0;

  do {
    c = rand_int(MAX_COLOUR_CODE);
  } while (c == GREY_ZERO || c == GREY_THREE);

  return c;
}

static void print_parrot()
{
  uint32_t c = get_colour();
  uint32_t f = get_colour();

  for (const char* s = PARROT; *s; s++) {
    switch (*s) {
      case 'C':
        printf("%d", c);
        break;
      case 'F':
        printf("%d", f);
        break;
      default:
        putchar(*s);
    }
  }
}

static inline void repeat(char* buf, char c, size_t times)
{
  if (times > 0) {
    memset(buf, c, times);
  }

  buf[times] = '\0';
}

static void print_balloon(char** lines, uint32_t line_count, uint32_t max_len)
{
  char* buffer = allocate(max_len + PADDING + 1);

  repeat(buffer, '_', max_len + PADDING);
  printf(" %s \n", buffer);

  for (uint32_t i = 0; i < line_count; i++) {
    const char* surrounds = SURROUNDS[0];

    if (line_count == 1) {
      surrounds = SURROUNDS[1];
    } else if (i == 0) {
      surrounds = SURROUNDS[2];
    } else if (i == line_count - 1) {
      surrounds = SURROUNDS[3];
    }

    printf("%c %-*s %c\n", surrounds[0], max_len, lines[i], surrounds[1]);
  }

  repeat(buffer, '-', max_len + PADDING);
  printf(" %s \n", buffer);

  free(buffer);
}

static char* slurp()
{
  size_t buffer_len = 256;
  char* buffer = allocate(buffer_len);

  int c = 0;
  uint32_t count = 0;

  while ((c = getchar()) != EOF) {
    if ((count + TABSHIFT) >= buffer_len) {
      buffer_len *= GOLDENISH_RATIO;
      buffer = resize(buffer, buffer_len);
    }

    if (c == '\t') {
      memset(buffer + count, ' ', TABSHIFT);
      count += TABSHIFT;
    } else if (iscntrl(c) && c != '\n') {
      buffer[count++] = '^';
      buffer[count++] = c ^ 0x40;
    } else {
      buffer[count++] = c;
    }
  }

  if (!feof(stdin)) {
    perror("Encountered error while reading from stdin");
    free(buffer);
    return NULL;
  }

  buffer[count] = '\0';
  return buffer;
}

static inline bool detect_no_colour()
{
  const char* no_colour = getenv("NO_COLOR");
  return (no_colour != NULL && no_colour[0] != '\0');
}

static int parrot(uint32_t width)
{
  if (detect_no_colour()) {
    fprintf(stderr, "Failed to detect color support or color support disabled\n");
    return EXIT_FAILURE;
  }

  char* text = slurp();

  if (text == NULL) {
    return EXIT_FAILURE;
  }

  uint32_t line_count = 0;
  uint32_t longest_line = 0;
  char** lines = wrap_text(text, width, &line_count, &longest_line);

  print_balloon(lines, line_count, longest_line);
  print_parrot();

  free(lines);
  free(text);

  return EXIT_SUCCESS; 
}

static inline int int_input(const char* in)
{
  char* err;
  unsigned long int out = strtoul(in, &err, 10);
  return (*err != '\0') ? -1 : out;
}

int main(int argc, char** argv)
{
  if (setlocale(LC_ALL, "") == NULL) {
    fprintf(stderr, "Failed to set locale\n");
    return EXIT_FAILURE;
  }

  const char* optstr = "vhw:";
  int width = DEFAULT_WIDTH;
  int opt;
  opterr = 0; /* Disable getopts default error to stderr */

  while ((opt = getopt(argc, argv, optstr)) != -1) {
    switch (opt) {
      case 'h':
        printf("%s", USAGE);
        return EXIT_SUCCESS;
      case 'v':
        printf("%s\n", VERSION);
        return EXIT_SUCCESS;
      case 'w':
        if ((width = int_input(optarg)) <= 0) {
          fprintf(stderr, "parrot: Invalid width provided (%s)\n", optarg);
          return EXIT_FAILURE;
        }
        break;
      case '?':
        if (strchr(optstr, optopt)) {
          fprintf(stderr, "parrot: Option -%c requires an argument\n", optopt);
        } else {
          fprintf(stderr, "parrot: Unknown option -%c\n", optopt);
        }
        fprintf(stderr, "Try 'parrot -h' for more information\n");
        return EXIT_FAILURE;
    }
  }

  return parrot(width);
}
