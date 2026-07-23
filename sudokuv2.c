#include <curses.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define W_WIDTH 46
#define W_HEIGHT 19

#define N 9
#define N_H_LINES 9
#define N_V_LINES 9

#define H_STEP 5
#define V_STEP 2

#define KEY_ESC 27
#define MAX_LINES 50

typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t i32;
typedef int64_t i64;

typedef struct {
  i32 x;
  i32 y;
} Position;

typedef struct {
  u16 row_mask[9];
  u16 col_mask[9];
  u16 box_mask[9];
} BitMasks;

typedef enum { EASY, MEDIUM, HARD, EXPERT } GameLevel;
typedef enum { MENU, GAME, HELP, EXIT } GameState;
typedef enum { CONTINUE, NEWGAME, HOWTOPLAY, MEXIT } MenuState;

typedef i32 Sudoku[N][N];

typedef struct {
  WINDOW *window;
  GameState state; // UI Based on which game state is active;
  bool isVisible;
} UIWindow;

typedef struct {
  char **titleAscii;
  int length;
} AsciiTitle;

typedef struct {
  Sudoku puzzle;
  Sudoku solution;
  Sudoku fixed;
  AsciiTitle title;
  GameState state;
  GameLevel gameLevel;
  MenuState menuState;
  bool isRunning;
} GameContext;

Position gPos = {0, 0}; // Tracks the relative coordinates i.e index
BitMasks gMask = {0};
i32 last_y = 1, last_x = 3; // Tracks the absolute global coordinates;
// utils
bool isDigit(char c) { return c >= 48 && c <= 57; }

// void draw(WINDOW *window, GameState state, GameContext *ctx);
void draw_grid(WINDOW *window, Sudoku grid, Sudoku fixed);
void draw_horizontal_lines(WINDOW *window);
void draw_vertical_lines(WINDOW *window);

void draw_menu(UIWindow *window, GameContext *ctx);
void draw_game(UIWindow *window, GameContext *ctx);
void draw_help(UIWindow *window, GameContext *ctx);

// inpute handlers
void handle_menu_input(i32 ch, GameContext *ctx);
void handle_game_input(i32 ch, GameContext *ctx);
void handle_help_input(char ch, GameContext *ctx);

// Game Input Hanlding
void move_right(Position *currPos, Sudoku fixed);
void move_left(Position *currPos);
void move_up(Position *currPos);
void move_down(Position *currPos);
void print_grid(WINDOW *window, Sudoku grid, Sudoku fixed);

// For Sudoku Generation and Digging
void fisher_yates_shuffle(i32 *arr, i32 n);
i32 is_safe(i32 grid[N][N], i32 row, i32 col, i32 num, BitMasks *masks);
void set_bit(BitMasks *masks, i32 row, i32 col, i32 num);
void clear_bit(BitMasks *masks, i32 row, i32 col, i32 num);
i32 get_box_idx(i32 row, i32 col);
i32 generate_sudoku(Sudoku grid, i32 row, i32 col);
void count_solutions(Sudoku grid, i32 row, i32 col, i32 *count, BitMasks mask);
void remove_cells(Sudoku grid, i32 n_cells_to_remove);

// Game Logic and helpers
int map_level_to_nclues(GameLevel lvl);

void set_game_state(GameState state, GameContext *ctx) { ctx->state = state; }
void insert_num(Sudoku grid, char ch);

bool init_menu_window(UIWindow *win);
bool init_game_window(UIWindow *win);
bool init_help_window(UIWindow *win);
bool init_exit_window(UIWindow *win);

void draw_title(WINDOW *window, GameContext *ctx) {
  AsciiTitle *title = &ctx->title;
  int y = 2;
  for (i32 i = 0; i < title->length; ++i) {
    mvwprintw(window, y++, (COLS - strlen(title->titleAscii[0])) / 2, "%s",
              title->titleAscii[i]);
  }
}

void draw(UIWindow *window, GameContext *ctx) {
  werase(stdscr);
  wnoutrefresh(stdscr);
  switch (ctx->state) {
  case MENU:
    curs_set(0);
    draw_menu(window, ctx);
    break;
  case GAME:
    curs_set(1);
    draw_game(window, ctx);
    break;
  case HELP:
    curs_set(0);
    draw_help(window, ctx);
    break;
  case EXIT:
    break;
  }

  draw_title(stdscr, ctx);
  wnoutrefresh(stdscr);
}

void handle_input(int ch, GameContext *ctx);

UIWindow *get_curr_active_window(GameContext *ctx, UIWindow *menuW,
                                 UIWindow *gameW, UIWindow *helpW,
                                 UIWindow *exitW);

char **read_title(const char *filename, int *numLines) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    fprintf(stderr, "Error: Couldn't read file %s", filename);
    return NULL;
  }

  char **lines = malloc(MAX_LINES * sizeof(*lines));
  if (!lines) {
    fclose(fp);
    return NULL;
  }

  char buffer[512];
  i32 count = 0;

  while (count < MAX_LINES && fgets(buffer, sizeof(buffer), fp)) {
    buffer[strcspn(buffer, "\n")] = '\0';
    lines[count] = malloc(strlen(buffer) + 1);
    strcpy(lines[count], buffer);
    count++;
  }

  fclose(fp);
  *numLines = count;
  return lines;
}

i32 main() {
  initscr();
  noecho();
  cbreak();
  curs_set(1);
  refresh();

  start_color();
  use_default_colors();          // Optional: preserve terminal background
  init_pair(1, COLOR_WHITE, -1); // Normal cells
  init_pair(2, COLOR_RED, -1);
  init_pair(3, COLOR_CYAN, -1);

  // Seeding the random generator
  srand(time(NULL));

  i32 midpoint = COLS / 2;
  int x, y;
  getmaxyx(stdscr, y, x);
  WINDOW *debugWindow;
  i32 debug_h = 5;
  debugWindow = newwin(debug_h, 60, LINES - debug_h - 1, midpoint - 60 / 2);

  keypad(debugWindow, true);

  UIWindow menuWindow, gameWindow, helpWindow, exitWindow;
  if (!init_menu_window(&menuWindow) || !init_game_window(&gameWindow) ||
      !init_help_window(&helpWindow) || !init_exit_window(&exitWindow)) {
    endwin();
    return -1;
  }

  int num_lines;
  char **titleAscii = read_title("title1.txt", &num_lines);

  AsciiTitle title = {titleAscii, num_lines};

  GameContext ctx = {.title = title, .state = MENU, MEDIUM, CONTINUE, true};

  Sudoku solution;
  generate_sudoku(solution, 0, 0);
  memcpy(ctx.solution, solution, sizeof(Sudoku));
  memcpy(ctx.puzzle, solution, sizeof(Sudoku));
  remove_cells(ctx.puzzle, map_level_to_nclues(ctx.gameLevel));
  memcpy(ctx.fixed, ctx.puzzle, sizeof(Sudoku));

  curs_set(0);

  i32 ch;
  WINDOW *titlewindow = newwin(num_lines + 2, strlen(titleAscii[0]) + 2, 2,
                               midpoint - strlen(titleAscii[0]) / 2 - 1);

  UIWindow *active = &menuWindow;
  draw(active, &ctx);
  doupdate();
  while (ctx.state != EXIT) {

    active = get_curr_active_window(&ctx, &menuWindow, &gameWindow, &helpWindow,
                                    &exitWindow);

    ch = wgetch(active->window);
    handle_input(ch, &ctx);

    active = get_curr_active_window(&ctx, &menuWindow, &gameWindow, &helpWindow,
                                    &exitWindow);

    draw(active, &ctx);
    // wclear(debugWindow);
    // box(debugWindow, 0, 0);
    //
    // mvwprintw(debugWindow, 1, 1, "%d, %d", gPos.y, gPos.x);
    // mvwprintw(debugWindow, 3, 1, "%d, %d", last_y, last_x);
    //
    // wnoutrefresh(debugWindow);
    doupdate();
  }

  printf("Exiting...");
  // delwin(window);
  endwin();

  return 0;
}
void draw_menu(UIWindow *window, GameContext *ctx) {
  werase(window->window);
  const char *menu[] = {
      "Continue",
      "New Game",
      "Help",
      "Exit",
  };

  mvwin(window->window, (LINES - W_HEIGHT) / 2, (COLS - W_WIDTH) / 2);
  i32 width = getmaxx(window->window);
  i32 height = getmaxy(window->window);

  // box(window, 0, 0);
  for (i32 i = 0; i < 4; i++) {
    i32 x = (width - strlen(menu[i])) / 2;
    i32 y = 20 + i * 2;
    if (i == ctx->menuState) {
      wattron(window->window, A_REVERSE);
      mvwaddch(window->window, y, x - 2, ' ');
      wattroff(window->window, A_REVERSE);

      wattron(window->window, COLOR_PAIR(2) | A_BOLD);
      mvwprintw(window->window, y, x, "%s", menu[i]);
      wattroff(window->window, COLOR_PAIR(2) | A_BOLD);

      wattron(window->window, A_REVERSE);
      mvwaddch(window->window, y, x + 1 + strlen(menu[i]), ' ');
      wattroff(window->window, A_REVERSE);
    } else {
      mvwprintw(window->window, y, x, "%s", menu[i]);
    }

    // if (i == selected)
    // wattroff(window, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
  }

  wnoutrefresh(window->window);
}

void draw_game(UIWindow *win, GameContext *ctx) {
  WINDOW *window = win->window;
  werase(window);
  mvwin(window, (LINES - W_HEIGHT) / 2, (COLS - W_WIDTH) / 2);
  draw_grid(window, ctx->puzzle, ctx->fixed);
  wmove(window, last_y, last_x);
  wnoutrefresh(window);
}
void draw_help(UIWindow *window, GameContext *ctx) { werase(window->window); }

void draw_grid(WINDOW *window, Sudoku grid, Sudoku fixed) {
  box(window, 0, 0);
  draw_horizontal_lines(window);
  draw_vertical_lines(window);
  print_grid(window, grid, fixed);
}

void draw_horizontal_lines(WINDOW *window) {
  for (i32 i = 1; i < N_H_LINES; i++) {
    mvwhline(window, i * V_STEP, 1, 0, W_WIDTH - 2);
  }
}
void draw_vertical_lines(WINDOW *window) {
  for (i32 i = 1; i < N_V_LINES; i++) {
    mvwvline(window, 1, i * H_STEP, 0, W_HEIGHT - 2);
  }
}

void print_grid(WINDOW *window, Sudoku sudokuGrid, Sudoku fixed) {
  for (i32 i = 0, x = 3; i < N; i++, x += H_STEP) {
    for (i32 j = 0, y = 1; j < N; j++, y += V_STEP) {
      bool is_fixed = fixed[j][i];
      if (is_fixed)
        wattron(window, A_BOLD | COLOR_PAIR(3));
      mvwaddch(window, y, x, sudokuGrid[j][i] ? sudokuGrid[j][i] + '0' : ' ');
      wattroff(window, A_BOLD | COLOR_PAIR(2));
    }
  }
  wmove(window, 1, 3);
}

void insert_num(Sudoku grid, char ch) { grid[gPos.y][gPos.x] = ch - '0'; }

void move_right(Position *pos, Sudoku fixed) {
  if (last_x + H_STEP < W_WIDTH - 1) {
    last_x += H_STEP;
    pos->x = last_x / H_STEP;
  }
  if (fixed[pos->x][pos->y])
    pos->x++;
}

void move_left(Position *pos) {
  if (last_x - H_STEP > 0) {
    last_x = last_x - H_STEP;
    pos->x = last_x / H_STEP;
  }
}

void move_up(Position *pos) {
  if (last_y - V_STEP > 0) {
    last_y = last_y - V_STEP;
    pos->y = last_y / V_STEP;
  }
}

void move_down(Position *pos) {
  if (last_y + V_STEP < W_HEIGHT) {
    last_y = last_y + V_STEP;
    pos->y = last_y / V_STEP;
  }
}

void fisher_yates_shuffle(i32 *arr, i32 n) {
  for (i32 i = n - 1; i >= 0; i--) {
    i32 j = rand() % (i + 1);
    i32 temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
  }
}

i32 is_safe(i32 grid[N][N], i32 row, i32 col, i32 num, BitMasks *masks) {
  u16 bitflag = 1 << (num - 1);
  return !(masks->row_mask[row] & bitflag) &&
         !(masks->col_mask[col] & bitflag) &&
         !(masks->box_mask[get_box_idx(row, col)] & bitflag);
}

void set_bit(BitMasks *masks, i32 row, i32 col, i32 num) {
  i32 bitflag = 1 << (num - 1);

  masks->row_mask[row] |= bitflag;
  masks->col_mask[col] |= bitflag;
  masks->box_mask[get_box_idx(row, col)] |= bitflag;
}

void clear_bit(BitMasks *masks, i32 row, i32 col, i32 num) {

  i32 bitflag = 1 << (num - 1);
  masks->row_mask[row] &= ~bitflag;
  masks->col_mask[col] &= ~bitflag;
  masks->box_mask[get_box_idx(row, col)] &= ~bitflag;
}
i32 get_box_idx(i32 row, i32 col) { return (row / 3) * 3 + (col / 3); }

i32 generate_sudoku(Sudoku sudokuGrid, i32 row, i32 col) {
  if (col == N) {
    row++;
    col = 0;
  }

  if (row == N) {
    return 1;
  }

  i32 nums[N] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  fisher_yates_shuffle(nums, N);
  for (i32 i = 0; i < N; i++) {
    i32 num = nums[i];

    if (is_safe(sudokuGrid, row, col, num, &gMask)) {
      sudokuGrid[row][col] = num;
      set_bit(&gMask, row, col, num);
      if (generate_sudoku(sudokuGrid, row, col + 1))
        return 1;
      clear_bit(&gMask, row, col, num);
    }
  }
  return 0;
}
void count_solutions(Sudoku b, i32 row, i32 col, i32 *count, BitMasks mask) {
  if (*count > 1)
    return;

  if (col == N) {
    row++;
    col = 0;
  }

  if (row == N) {
    (*count)++;
    return;
  }

  if (b[row][col] != 0) {
    count_solutions(b, row, col + 1, count, mask);
    return;
  }

  i32 box = get_box_idx(row, col);

  for (i32 num = 1; num <= 9; ++num) {
    u16 bitflag = 1 << (num - 1);
    if (!(mask.row_mask[row] & bitflag) && !(mask.col_mask[col] & bitflag) &&
        !(mask.box_mask[box] & bitflag)) {

      BitMasks next_mask = mask;
      set_bit(&next_mask, row, col, num);
      count_solutions(b, row, col + 1, count, next_mask);
    }
  }
}

// Digging
void remove_cells(Sudoku b, i32 n_cells_to_remove) {
  i32 cells[N * N];
  for (i32 i = 0; i < N * N; ++i)
    cells[i] = i;

  fisher_yates_shuffle(cells, N * N);

  i32 removed = 0;
  for (i32 i = 0; i < N * N && removed < n_cells_to_remove; i++) {
    i32 row = cells[i] / N;
    i32 col = cells[i] % N;

    if (b[row][col] == 0)
      continue;

    i32 backup = b[row][col];
    u16 bitflag = 1 << (backup - 1);
    i32 box = get_box_idx(row, col);

    b[row][col] = 0;
    clear_bit(&gMask, row, col, backup);

    i32 count = 0;
    count_solutions(b, 0, 0, &count, gMask);

    if (count != 1) {
      // multiple soluton exists so back up
      b[row][col] = backup;
      set_bit(&gMask, row, col, backup);
    } else {
      removed++;
    }
  }
}

i32 map_level_to_nclues(GameLevel lvl) {
  i32 minClues, maxClues;
  switch (lvl) {
  case EASY:
    minClues = 36;
    maxClues = 40;
    break;
  case MEDIUM:
    minClues = 30;
    maxClues = 35;
    break;
  case HARD:
    minClues = 25;
    maxClues = 29;
    break;
  case EXPERT:
    minClues = 20;
    maxClues = 24;
    break;
  default:
    minClues = 30;
    maxClues = 35;
    break;
  }

  i32 nClues = minClues + rand() % (maxClues - minClues + 1);
  return N * N - nClues;
}

void handle_game_input(int ch, GameContext *ctx) {
  if (isDigit(ch)) {
    insert_num(ctx->puzzle, ch);
  } else {
    switch (ch) {
    case 'q':
      ctx->state = MENU;
      break;
    case KEY_RIGHT:
    case 'l':
    case 'L':
      move_right(&gPos, ctx->fixed);
      break;
    case KEY_LEFT:
    case 'h':
    case 'H':
      move_left(&gPos);
      break;
    case KEY_UP:
    case 'k':
    case 'K':
      move_up(&gPos);
      break;
    case KEY_DOWN:
    case 'j':
    case 'J':
      move_down(&gPos);
      break;
    case KEY_BACKSPACE:
      insert_num(ctx->puzzle, '0');
      break;
    default:
      break;
    }
  }
}

void handle_menu_input(i32 ch, GameContext *ctx) {
  switch (ch) {
  case 'q':
  case 'Q':
    ctx->state = EXIT;
    break;
  case 'j':
  case 'J':
  case KEY_DOWN:
    if (ctx->menuState < MEXIT)
      ctx->menuState++;
    else
      ctx->menuState = CONTINUE;
    break;
  case 'k':
  case 'K':
  case KEY_UP:
    if (ctx->menuState > CONTINUE)
      ctx->menuState--;
    else
      ctx->menuState = MEXIT;
    break;

  case '\n':
    switch (ctx->menuState) {
    case NEWGAME:
      ctx->state = GAME;
      break;
    case HOWTOPLAY:
      ctx->state = HELP;
      break;
    case MEXIT:
      ctx->state = EXIT;
      break;
    default:
      break;
    }
    break;
  }
}
void handle_help_input(char ch, GameContext *ctx) {
  switch (ch) {
  case 'q':
  case 'Q':
    ctx->state = MENU;
    break;
  }
}

bool init_menu_window(UIWindow *win) {
  if (win == NULL)
    return false;
  i32 x, y;
  getmaxyx(stdscr, y, x);
  win->window = newwin(y, x, 0, 0);
  if (win->window == NULL)
    return false;
  win->isVisible = true;
  win->state = MENU;
  keypad(win->window, true);
  return true;
}
bool init_game_window(UIWindow *win) {
  if (win == NULL)
    return false;

  i32 midpoint = COLS / 2;

  win->window = newwin(W_HEIGHT, W_WIDTH, 1, midpoint - (W_WIDTH) / 2);
  if (win->window == NULL)
    return false;
  win->isVisible = false;
  win->state = GAME;
  keypad(win->window, true);
  return true;
}

bool init_help_window(UIWindow *win) {
  if (win == NULL)
    return false;

  i32 x, y;
  getmaxyx(stdscr, y, x);
  win->window = newwin(y, x, 0, 0);
  if (win->window == NULL)
    return false;
  win->isVisible = false;
  win->state = HELP;
  keypad(win->window, true);

  return true;
}
bool init_exit_window(UIWindow *win) { return true; }

void handle_input(i32 ch, GameContext *ctx) {
  switch (ctx->state) {
  case MENU:
    handle_menu_input(ch, ctx);
    break;
  case GAME:
    handle_game_input(ch, ctx);
    break;
  case HELP:
    handle_help_input(ch, ctx);
    break;
  case EXIT:
    break;
  }
}

UIWindow *get_curr_active_window(GameContext *ctx, UIWindow *menuW,
                                 UIWindow *gameW, UIWindow *helpW,
                                 UIWindow *exitW) {
  switch (ctx->state) {
  case MENU:
    return menuW;
  case GAME:
    return gameW;
  case HELP:
    return helpW;
  default:
    return exitW;
  }
}
