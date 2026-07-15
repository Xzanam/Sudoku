#include <ncurses.h>
#include <stdio.h>
#define W_WIDTH 46
#define W_HEIGHT 19

#define N 9
#define N_H_LINES 9
#define N_V_LINES 9

#define H_STEP 5
#define V_STEP 2

int last_y = 1, last_x = 3;
// utils
bool isDigit(char c) { return c >= 48 && c <= 57; }

void draw_grid(WINDOW *window);
void draw_horizontal_lines(WINDOW *window);
void draw_vertical_lines(WINDOW *window);

int sudokuGrid[N][N] = {0};

struct Coord {
  int x;
  int y;
} gPos = {0, 0};

void move_right(struct Coord *currPos);
void move_left(struct Coord *currPos);
void move_up(struct Coord *currPos);
void move_down(struct Coord *currPos);
void print_grid(WINDOW *window);

// void addCh(WINDOW *window, char ch);
void addCh(char ch);

int main() {
  initscr();
  noecho();
  cbreak();
  curs_set(1);

  WINDOW *window, *debugWindow;

  int midpoint = COLS / 2;
  bool isRunning = true;

  window = newwin(W_HEIGHT, W_WIDTH, 1, midpoint - (W_WIDTH) / 2);

  int debug_h = 5;
  debugWindow = newwin(debug_h, 60, LINES - debug_h - 1, midpoint - 60 / 2);

  keypad(window, true);
  keypad(debugWindow, true);
  if (window == NULL) {
    endwin();
    return -1;
  }

  draw_grid(window);
  int ch;
  while (isRunning) {
    //
    // wnoutrefresh(debugWindow);
    // wnoutrefresh(window);
    //
    // wmove(window, last_y, last_x);
    // doupdate();

    ch = wgetch(window);

    wclear(debugWindow);
    box(debugWindow, 0, 0);

    if (isDigit(ch)) {
      mvwprintw(debugWindow, 2, 2, "It's a digit");
      addCh(ch);
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

    draw_grid(window);

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

void draw_grid(WINDOW *window) {
  box(window, 0, 0);
  draw_horizontal_lines(window);
  draw_vertical_lines(window);
  print_grid(window);
}

void draw_horizontal_lines(WINDOW *window) {
  for (int i = 1; i < N_H_LINES; i++) {
    mvwhline(window, i * V_STEP, 1, 0, W_WIDTH - 2);
  }
}
void draw_vertical_lines(WINDOW *window) {
  for (int i = 1; i < N_V_LINES; i++) {
    mvwvline(window, 1, i * H_STEP, 0, W_HEIGHT - 2);
  }
}

void print_grid(WINDOW *window) {
  for (int i = 0, x = 3; i < N; i++, x += H_STEP) {
    for (int j = 0, y = 1; j < N; j++, y += V_STEP) {
      mvwaddch(window, y, x, sudokuGrid[j][i] ? sudokuGrid[j][i] + '0' : ' ');
    }
  }
  wmove(window, 1, 3);
}

// void addCh(WINDOW *window, char ch) { waddch(window, ch); }
void addCh(char ch) { sudokuGrid[gPos.y][gPos.x] = ch - '0'; }

void move_right(struct Coord *coord) {
  if (last_x + H_STEP < W_WIDTH - 1) {
    last_x += H_STEP;
    coord->x = last_x / H_STEP;
  }

  // wmove(window, y, x + H_STEP);
}

void move_left(struct Coord *coord) {
  if (last_x - H_STEP > 0) {
    last_x = last_x - H_STEP;
    coord->x = last_x / H_STEP;
  }

  // wmove(window, y, x - H_STEP);
}

void move_up(struct Coord *coord) {
  if (last_y - V_STEP > 0) {
    last_y = last_y - V_STEP;
    coord->y = last_y / V_STEP;
  }
  // wmove(window, y - V_STEP, x);
}

void move_down(struct Coord *coord) {
  if (last_y + V_STEP < W_HEIGHT) {
    last_y = last_y + V_STEP;
    coord->y = last_y / V_STEP;
  }
  // wmove(window, y + V_STEP, x);
}
