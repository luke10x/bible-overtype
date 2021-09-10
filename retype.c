/*
 * while inotifywait -e close_write retype.c; do gcc -Wall -o retype retype.c $(ncursesw5-config --cflags --libs) ; done 
 */

#include <stdio.h>
#include <stdint.h>
#include <locale.h>
#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <time.h>
#include <inttypes.h>
#include <wchar.h>
// #include <wchar.h>


// #include <unitypes.h>
#include <uniconv.h>
// #include <unistdio.h>
// #include <unistr.h>
// #include <uniwidth.h>
// #include <utf8proc.h>

#define GREY_PAIR    5
#define GOOD_PAIR    6
#define ERROR_PAIR    7

int is_move_okay (int y, int x);
void draw_map (void);
int get_padding (int longest_line, int term_cols);
void print_grey (const int row, const int col, const char *line);
void re_type (const int row, const int col, const char *line,
	      const int offset, const struct winsize w);
static void sig_handler (int sig);

#define MAX_LEN 256
WINDOW *pad;

int
main (void)
{
  signal (SIGWINCH, sig_handler);

  int longest_line = 0;
  
  /*
   * int lines_total = 0; 
   */
  char ch_arr[MAX_LEN][MAX_LEN];

  FILE *fp;
  // fp = fopen("teve-musu.txt", "r");
  fp = stdin;

  if (fp == NULL)
    {
      perror ("Failed: ");
      return 1;
    }

  char buffer[MAX_LEN];
  int i = 0;
  while (fgets (buffer, MAX_LEN - 1, fp))
    {
      // Remove trailing newline
      buffer[strcspn (buffer, "\n")] = 0;
      /*
       * printf("%d %s\n", i, buffer); 
       */
      strcpy (ch_arr[i], buffer);
      i++;

      if (strlen (buffer) > longest_line)
	{
	  longest_line = strlen (buffer);
	}
    }

  fclose (fp);

  int lines_total = i;

  freopen ("/dev/tty", "rw", stdin);
  setlocale (LC_ALL, "");

  initscr ();
  struct winsize w;
  ioctl (0, TIOCGWINSZ, &w);

  int padding = get_padding (longest_line, w.ws_col);
  keypad (stdscr, TRUE);
  cbreak ();
  noecho ();

  /*
   * initialize colors 
   */
  if (has_colors () == FALSE)
    {
      endwin ();
      printf ("Your terminal does not support color\n");
      exit (1);
    }

  start_color ();
  init_pair (GREY_PAIR, COLOR_WHITE, COLOR_BLACK);
  init_pair (ERROR_PAIR, COLOR_RED + 8, COLOR_YELLOW + 8);
  init_pair (GOOD_PAIR, COLOR_GREEN + 8, COLOR_BLACK);
  clear ();

  pad = newpad (lines_total, w.ws_col);

  const int lines_done = 0;

  for (i = 0; i < lines_done; i++)
    {
      print_grey (i, padding, ch_arr[i]);
    }

  time_t start_time;
  time_t end_time;

  start_time = time (NULL);
  for (i = lines_done; i < lines_total; i++)
    {
      print_grey (i, padding, ch_arr[i]);
      const int offset = (i < w.ws_row) ? 0 : i - w.ws_row + 1;

      refresh ();
      prefresh (pad, offset, 0, 0, 0, w.ws_row - 1, w.ws_col - 1);

      wmove (pad, offset, padding);

      refresh ();
      prefresh (pad, offset, 0, 0, 0, w.ws_row - 1, w.ws_col - 1);

      re_type (i, padding, ch_arr[i], offset, w);
    }
  end_time = time (NULL);

  const double diff_t = difftime (end_time, start_time);
  printf ("Execution time = %f\n", diff_t);


  getch ();
  endwin ();
  exit (0);
}

int
get_padding (int longest_line, int term_cols)
{
  return (term_cols - longest_line) / 2;
}

void
print_grey (const int row, const int col, const char *line)
{
  wmove (pad, row, col);
  attron (COLOR_PAIR (GREY_PAIR));
  waddstr (pad, line);

  attroff (COLOR_PAIR (GREY_PAIR));
}

struct charstack
{
  wint_t ch;
  struct charstack *next;
};


typedef uint8_t utf8_t;

int
isssame (const int expected_ch, const int ch)
{
  fwprintf (stderr, L"%d == %d \n", expected_ch, ch);

  if (expected_ch == ch)
    {
      return TRUE;
    }
  return FALSE;
  // printf("", testch);
  // utf8_t *output;
  // ssize_t len = 

  // int input = wcval;


  // utf8proc_map((uint8_t*)&wcval, 0, &output, 
  // UTF8PROC_NULLTERM | UTF8PROC_STABLE |
  // UTF8PROC_STRIPMARK | UTF8PROC_DECOMPOSE |
  // UTF8PROC_CASEFOLD
  // );
  // fprintf(stderr, "SHE is output [%s]\n", output);

  // if (testch == *output) {
  // return TRUE;
  // }
  // return FALSE; 


}

int
simplify (const char *line, const int index)
{

  const uint32_t *mbcs = u32_strconv_from_locale (line);
  // fwprintf(stderr, L"HI %d\n", mbcs[0]);
  // fwprintf(stderr, L"HI %s\n", unicode_to_utf8(mbcs[1]));
  // fwprintf(stderr, L"HI %d\n", mbcs[2]);
  // fwprintf(stderr, L"HI %d\n", mbcs[3]);
  // fwprintf(stderr, L"HI %d\n", mbcs[4]);


  // unsigned char *output = malloc(8000) ;

  // utf8proc_map((const unsigned char *)(&mbcs[index]), strlen(line),
  // &output, 
  // UTF8PROC_NULLTERM | UTF8PROC_STABLE |
  // UTF8PROC_STRIPMARK | UTF8PROC_DECOMPOSE | UTF8PROC_STRIPMARK |
  // UTF8PROC_CASEFOLD);

  const int unicode = (int) *(&mbcs[index]);
  // fwprintf(stderr, L"%d. input: %ls(%d) Output: %ls(%d)\n", index,
  // origch, (int)*(&mbcs[index]), output, *output);
  // fwprintf(stderr, L"%d. (%d)\n", index, unicode);
  return unicode;

}

void
re_type (const int row, const int col, const char *line, const int offset,
	 const struct winsize w)
{
  struct charstack *undostack = NULL;
  wmove (pad, row, col);
  refresh ();
  prefresh (pad, offset, 0, 0, 0, w.ws_row - 1, w.ws_col - 1);

  int typed = 0;
  do
    {
      // char ch;
      wint_t typed_ch = getwchar ();

      // endwin();
      // printf("The iddnteger is: %d\n",(int) ch);
      // putwchar(ch);

      // ch = ungetwc(ch, stdin);
      // ch = getch();
      // printf( "%d\n - UNPRESSED\n", ch);
      // getch();
      // exit(1);
      if (typed_ch == 27)
	{
	  endwin ();
	  exit (0);
	}
      if (typed_ch == 13)
	{

	  fwprintf (stderr, L"typed: %d\n", typed);
	  fwprintf (stderr, L"normal length: %zu\n", strlen (line));

	  // char *pmb = (char *)malloc( MB_CUR_MAX );
	  // wchar_t *pwcs = (wchar_t *)malloc( sizeof( wchar_t ));


	  // // int len = wcstombs( pmb, line, MB_CUR_MAX);
	  // mbstowcs( pwcs, line, MB_CUR_MAX);

	  int i = 0;
	  while (simplify (line, i) != 0)
	    {
	      i++;
	    }


	  fwprintf (stderr, L"smart length: %zu\n", i);
	  if (typed < i)
	    {
	      continue;
	    }
	  break;
	}

      if (typed_ch == 127)
	{
	  if (undostack == NULL)
	    {
	      continue;
	    }
	  int x, y;
	  getyx (pad, y, x);

	  // Pop from undo stack
	  struct charstack *temp;
	  temp = undostack;
	  undostack = undostack->next;

	  wattron (pad, COLOR_PAIR (GREY_PAIR));
	  wmove (pad, y, x - 1);
	  waddch (pad, temp->ch);
	  wattroff (pad, COLOR_PAIR (GREY_PAIR));

	  // Again we need to go back becuase we just printed popped
	  // char
	  wmove (pad, y, x - 1);
	  refresh ();
	  prefresh (pad, offset, 0, 0, 0, w.ws_row - 1, w.ws_col - 1);
	  continue;
	}

      int expected_ch = simplify (line, typed);
      // fwprintf(stderr, L"%d. %lsc(%d) TEST: %ls(%d)\n", typed,
      // expected_ch, (int)expected_ch);
      fwprintf (stderr, L"%d. TEST:(%d) \n", typed, expected_ch);

      const int same = isssame (expected_ch, typed_ch);
      // const char testch = winch(pad) & A_CHARTEXT;
      if (same && undostack == NULL)
	{
	  wattron (pad, COLOR_PAIR (GOOD_PAIR));
	  // waddnstr(pad, (const char *)&typed_ch, sizeof(wint_t));
	  // sputwchar(typed_ch);
	  // waddstr(pad, wsprintf(L"%lc", expected_ch));

	  char str[80];
	  sprintf (str, "%lc", expected_ch);
	  waddstr (pad, str);

	  wattroff (pad, COLOR_PAIR (GOOD_PAIR));

	  refresh ();
	  prefresh (pad, offset, 0, 0, 0, w.ws_row - 1, w.ws_col - 1);
	  typed++;
	}
      else
	{
	  struct charstack *nptr = malloc (sizeof (struct charstack));
	  nptr->ch = expected_ch;
	  nptr->next = undostack;
	  undostack = nptr;

	  wattron (pad, COLOR_PAIR (ERROR_PAIR));

	  char str[80];
	  sprintf (str, "%lc", typed_ch);
	  waddstr (pad, str);

	  // waddch(pad, typed_ch);

	  wattroff (pad, COLOR_PAIR (ERROR_PAIR));

	  refresh ();
	  prefresh (pad, offset, 0, 0, 0, w.ws_row - 1, w.ws_col - 1);
	}
    }
  while (true);
}

static void
sig_handler (int sig)
{
  if (SIGWINCH == sig)
    {
      struct winsize winsz;

      ioctl (0, TIOCGWINSZ, &winsz);
      printf ("SIGWINCH raised, window size: %d rows / %d columns\n",
	      winsz.ws_row, winsz.ws_col);
    }
}				// sig_handler 
