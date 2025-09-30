#include <ncurses.h>
#include <stdio.h>

#define W_WIDTH 46
#define W_HEIGHT 19

#define N_H_LINES 8
#define N_V_LINES 8

void draw_grid(WINDOW *window);
void draw_horizontal_lines(WINDOW *window);
void draw_vertical_lines(WINDOW *window);

void move_right(WINDOW *window);
void move_left(WINDOW *window);
void move_up(WINDOW *window);
void move_down(WINDOW *window);

int main() {
  initscr();
  noecho();
  cbreak();
  keypad(stdscr, true);
  curs_set(1);

  WINDOW *window;

  int midpoint = COLS / 2;
  bool isRunning = true;

  window = newwin(W_HEIGHT, W_WIDTH, 1, midpoint - (W_WIDTH) / 2);
  if (window == NULL) {
    endwin();
    return -1;
  }

  int ch;

  draw_grid(window);
  wmove(window, 1, 3);
  while (isRunning) {
    ch = getch();
    switch (ch) {
    case 'q':
      isRunning = false;
      break;
    case KEY_RIGHT:
      move_right(window);
      break;
    case KEY_LEFT:
      move_left(window);
      break;
    case KEY_UP:
      move_up(window);
      break;
    case KEY_DOWN:
      move_down(window);
      break;
    default:
      break;
    }
    wrefresh(window);
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
}

void draw_horizontal_lines(WINDOW *window) {
  for (int i = 0; i <= N_H_LINES; i++) {
    mvwhline(window, i * 2, 1, 0, W_WIDTH - 2);
  }
}
void draw_vertical_lines(WINDOW *window) {
  for (int i = 0; i <= N_V_LINES; i++) {
    mvwvline(window, 1, i * 5, 0, W_HEIGHT - 2);
  }
}

void move_right(WINDOW *window) {
  int y, x;
  getyx(window, y, x);
  wmove(window, y, x + 5);
}
void move_left(WINDOW *window) {
  int y, x;
  getyx(window, y, x);
  wmove(window, y, x - 5);
}

void move_up(WINDOW *window) {
  int y, x;
  getyx(window, y, x);
  wmove(window, y - 2, x);
}

void move_down(WINDOW *window) {
  int y, x;
  getyx(window, y, x);
  wmove(window, y + 2, x);
}
