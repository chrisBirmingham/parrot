#include <ctype.h>
#include <getopt.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Pull in external getopt globals */
extern char* optarg;
extern int optind;
extern int optopt;
extern int opterr;

static const char* VERSION = "1.0.0\n";

static const char* USAGE = "Usage: parrot [OPTION]...\n"
"Repeats whatever you tell it.\n"
"\n"
"   -w  Set width of the text balloon\n"
"   -v  Print version and exit\n"
"   -h  Show this message and exit\n";

static const char* MEMORY_ALLOC_MSG = "Failed to allocate memory";

static const int DEFAULT_WIDTH = 40;
static const int TABSHIFT = 8;

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

static void* allocate(size_t size)
{
  void* tmp = malloc(size);

  if (tmp == NULL) {
    perror(MEMORY_ALLOC_MSG);
    exit(EXIT_FAILURE);
  }

  return tmp;
}

static void* resize(void* ptr, size_t size)
{
  void* tmp = realloc(ptr, size);

  if (tmp == NULL) {
    perror(MEMORY_ALLOC_MSG);
    exit(EXIT_FAILURE);
  }

  return tmp;
}

static int rand_int(int max_int)
{
  /**
   * Attempt to reduce modulo bias in standard C rand
   * https://stackoverflow.com/questions/10984974/why-do-people-say-there-is-modulo-bias-when-using-a-random-number-generator
   */
  int r;
  int range = RAND_MAX - (((RAND_MAX % max_int) + 1) % max_int);

  do {
    r = rand();
  } while (r > range);

  return r % max_int + 1;
}

static unsigned int u8strlen(const char *s)
{
  unsigned int len = 0;

  for (; *s; s++) {
    len += ((*s & 0xC0) != 0x80);
  }

  return len;
}

static inline int max(int lhs, int rhs)
{
  return (lhs > rhs)? lhs : rhs;
}

static char** wrap_text(
  char* str,
  unsigned int width,
  unsigned int* restrict line_count,
  unsigned int* restrict longest_line
) {
  unsigned int count = 0;
  unsigned int max_line = 0;
  char* last_space = 0;
  char* line_start = str;
  char* p;

  size_t lines_size = 10;
  char** lines = allocate(lines_size * sizeof(char*));

  for (p = str; *p; p++) {
    if (*p == '\n') {
      *p = '\0';
      lines[count++] = line_start;
      max_line = max(max_line, u8strlen(line_start));
      line_start = p + 1;
    }

    if (*p == ' ') {
      last_space = p;
    }

    if (p - line_start > width && last_space) {
      *last_space = '\0';
      lines[count++] = line_start;
      max_line = max(max_line, u8strlen(line_start));
      line_start = last_space + 1;
      last_space = 0;
    }

    if (count == lines_size) {
      lines_size *= 2;
      lines = resize(lines, lines_size * sizeof(char*));
    }
  }

  if (p > line_start) {
    lines[count++] = line_start;
    max_line = max(max_line, u8strlen(line_start));
  }

  *longest_line = max_line;
  *line_count = count;
  return lines;
}

static int get_colour()
{
  int c = 0;

  do {
    c = rand_int(MAX_COLOUR_CODE);
  } while (c == GREY_ZERO || c == GREY_THREE);

  return c;
}

static void print_parrot()
{
  int c = get_colour();
  int f = get_colour();

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

static inline void repeat(char* buf, char c, int times)
{
  if (times > 0) {
    memset(buf, c, times);
  }

  buf[times] = '\0';
}

static void print_balloon(char** lines, unsigned int line_count, unsigned int max_len)
{
  char* buffer = allocate(max_len + PADDING + 1);

  repeat(buffer, '_', max_len + PADDING);
  printf(" %s \n", buffer);

  for (int i = 0; i < line_count; i++) {
    const char* line = lines[i];
    const char* surrounds = SURROUNDS[0];

    if (line_count == 1) {
      surrounds = SURROUNDS[1];
    } else if (i == 0) {
      surrounds = SURROUNDS[2];
    } else if (i == line_count - 1) {
      surrounds = SURROUNDS[3];
    }

    printf("%c %-*s %c\n", surrounds[0], max_len, line, surrounds[1]);
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
  int count = 0;

  while ((c = fgetc(stdin)) != EOF) {
    if ((count + TABSHIFT) >= buffer_len) {
      buffer_len *= 2;
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

static int detect_colour_support()
{
  const char* no_colour = getenv("NO_COLOR");

  if (no_colour != NULL && no_colour[0] != '\0') {
    return -1;
  }

  const char* colour_term = getenv("COLORTERM");

  if (colour_term != NULL &&
    (strcmp(colour_term, "truecolor") == 0 || strcmp(colour_term, "24bit") == 0)) {
      return 0;
  }

  const char* term = getenv("TERM");

  if (term == NULL || strcmp(term, "dumb") == 0) {
    return -1;
  }

  return (strstr(term, "-256color") != NULL || strstr(term, "-truecolor") != NULL);
}

static int parrot(unsigned int width)
{
  if (detect_colour_support() < 0) {
    fprintf(stderr, "Failed to detect color support or color support disabled\n");
    return EXIT_FAILURE;
  }

  srand(time(NULL) + getpid());

  char* text = slurp();

  if (text == NULL) {
    return EXIT_FAILURE;
  }

  unsigned int line_count;
  unsigned int longest_line;
  char** lines = wrap_text(text, width, &line_count, &longest_line);

  print_balloon(lines, line_count, longest_line);

  print_parrot();

  free(lines);
  free(text);

  return EXIT_SUCCESS; 
}

static int int_input(const char* in)
{
  char* err;
  unsigned long int out = strtoul(in, &err, 10);

  if (*err != '\0') {
    return -1;
  }

  return out;
}

int main(int argc, char** argv)
{
  if (setlocale(LC_ALL, "") == NULL) {
    fprintf(stderr, "Failed to set locale\n");
    return EXIT_FAILURE;
  }

  int width = DEFAULT_WIDTH;
  int opt;

  opterr = 0; /* Disable getopts default error to stderr */
  while ((opt = getopt(argc, argv, "vhw:")) != -1) {
    switch (opt) {
      case 'h':
        printf("%s", USAGE);
        return EXIT_SUCCESS;
      case 'v':
        printf("%s", VERSION);
        return EXIT_SUCCESS;
      case 'w':
        width = int_input(optarg);
        if (width < 0) {
          fprintf(stderr, "parrot: Invalid width provided (%s)\n", optarg);
          return EXIT_FAILURE;
        }
        break;
      case '?':
        if (optopt == 'w') {
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
