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

typedef i32 Sudoku[N][N];

Position gPos = {0, 0}; // Tracks the relative coordinates i.e index
BitMasks g_mask = {0};

i32 last_y = 1, last_x = 3; // Tracks the absolute global coordinates;
// utils
bool isDigit(char c) { return c >= 48 && c <= 57; }

void draw_grid(WINDOW *window, Sudoku grid);
void draw_horizontal_lines(WINDOW *window);
void draw_vertical_lines(WINDOW *window);

void move_right(Position *currPos);
void move_left(Position *currPos);
void move_up(Position *currPos);
void move_down(Position *currPos);
void print_grid(WINDOW *window, Sudoku grid);

// For Sudoku Generation and Digging
void fisher_yates_shuffle(i32 *arr, i32 n);
i32 is_safe(i32 grid[N][N], i32 row, i32 col, i32 num, BitMasks *masks);
void set_bit(BitMasks *masks, i32 row, i32 col, i32 num);
void clear_bit(BitMasks *masks, i32 row, i32 col, i32 num);
i32 get_box_idx(i32 row, i32 col);
i32 generate_sudoku(Sudoku grid, i32 row, i32 col);
void count_solutions(Sudoku grid, i32 row, i32 col, i32 *count, BitMasks mask);
void remove_cells(Sudoku grid, i32 n_cells_to_remove);

// void addCh(WINDOW *window, char ch);
void insert_num(Sudoku grid, char ch);

i32 main() {
  initscr();
  noecho();
  cbreak();
  curs_set(1);

  // Seeding the random generator
  srand(time(NULL));

  WINDOW *window, *debugWindow;
  Sudoku solution = {0}, puzzle = {0};

  generate_sudoku(solution, 0, 0);
  memcpy(puzzle, solution, sizeof(Sudoku));
  remove_cells(puzzle, 60);

  i32 midpoint = COLS / 2;
  bool isRunning = true;
  window = newwin(W_HEIGHT, W_WIDTH, 1, midpoint - (W_WIDTH) / 2);

  i32 debug_h = 5;
  debugWindow = newwin(debug_h, 60, LINES - debug_h - 1, midpoint - 60 / 2);

  keypad(window, true);
  keypad(debugWindow, true);

  if (window == NULL) {
    endwin();
    return -1;
  }

  draw_grid(window, puzzle);
  i32 ch;
  while (isRunning) {

    ch = wgetch(window);

    wclear(debugWindow);
    box(debugWindow, 0, 0);

    if (isDigit(ch)) {
      mvwprintw(debugWindow, 2, 2, "It's a digit");
      insert_num(puzzle, ch);
      // addCh(window, ch);
    } else {
      switch (ch) {
      case 'q':
        isRunning = false;
        break;
      case KEY_RIGHT:
      case 'l':
      case 'L':
        move_right(&gPos);
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
      default:
        break;
      }
    }

    draw_grid(window, puzzle);

    mvwprintw(debugWindow, 1, 1, "%d, %d", gPos.y, gPos.x);
    mvwprintw(debugWindow, 3, 1, "%d, %d", last_y, last_x);

    wnoutrefresh(debugWindow);
    wnoutrefresh(window);

    wmove(window, last_y, last_x);
    doupdate();
  }

  printf("Exiting...");
  delwin(window);
  endwin();

  return 0;
}

void draw_grid(WINDOW *window, Sudoku grid) {
  box(window, 0, 0);
  draw_horizontal_lines(window);
  draw_vertical_lines(window);
  print_grid(window, grid);
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

void print_grid(WINDOW *window, Sudoku sudokuGrid) {
  for (i32 i = 0, x = 3; i < N; i++, x += H_STEP) {
    for (i32 j = 0, y = 1; j < N; j++, y += V_STEP) {
      mvwaddch(window, y, x, sudokuGrid[j][i] ? sudokuGrid[j][i] + '0' : ' ');
    }
  }
  wmove(window, 1, 3);
}

void insert_num(Sudoku grid, char ch) { grid[gPos.y][gPos.x] = ch - '0'; }

void move_right(Position *pos) {
  if (last_x + H_STEP < W_WIDTH - 1) {
    last_x += H_STEP;
    pos->x = last_x / H_STEP;
  }
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

    if (is_safe(sudokuGrid, row, col, num, &g_mask)) {
      sudokuGrid[row][col] = num;
      set_bit(&g_mask, row, col, num);
      if (generate_sudoku(sudokuGrid, row, col + 1))
        return 1;
      clear_bit(&g_mask, row, col, num);
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
    clear_bit(&g_mask, row, col, backup);

    i32 count = 0;
    count_solutions(b, 0, 0, &count, g_mask);

    if (count != 1) {
      // multiple soluton exists so back up
      b[row][col] = backup;
      set_bit(&g_mask, row, col, backup);
    } else {
      removed++;
    }
  }
}
