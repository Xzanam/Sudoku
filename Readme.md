# Terminal Sudoku

A terminal-based Sudoku game written in **C** using **ncurses**.

The project started as a simple Sudoku implementation while learning C and has been developed into an interactive terminal UI with keyboard navigation, puzzle generation, difficulty levels, save/load support, fixed-cell tracking, and a Unicode/ACS-based grid.

## Features

- Interactive terminal UI using ncurses
- Sudoku puzzle generation using backtracking
- Configurable difficulty levels:
  - Easy
  - Medium
  - Hard
  - Expert
- Puzzle digging/removal while checking that the generated puzzle has a unique solution
- Bitmask-based Sudoku constraint tracking
- Keyboard navigation through editable cells
- Fixed cells are skipped during navigation
- Save and load game state
- Separate menu, game, and help UI states
- Color support for fixed and incorrect/user-entered cells
- Unicode/wide-character support for custom grid styles

## Controls

### Menu

| Key | Action |
|---|---|
| `j` / `↓` | Move down |
| `k` / `↑` | Move up |
| `Enter` | Select |
| `q` | Quit |

### Game

| Key | Action |
|---|---|
| `h` / `←` | Move left |
| `j` / `↓` | Move down |
| `k` / `↑` | Move up |
| `l` / `→` | Move right |
| `1` - `9` | Insert a number |
| `Backspace` | Clear current cell |
| `q` | Save and return to menu |

Navigation automatically skips fixed cells.

## Sudoku Generation

The Sudoku generator uses recursive backtracking.

For each cell, the generator:

1. Creates a randomized ordering of the numbers `1` through `9`.
2. Checks whether a number is safe in the current row, column, and 3×3 box.
3. Places the number.
4. Recursively continues to the next cell.
5. Backtracks when no valid number can be placed.

Instead of scanning the entire row, column, and box for every candidate, the generator maintains bitmasks:

```text
row_mask[9]
col_mask[9]
box_mask[9]
```

Each bit represents one of the numbers `1` through `9`.

For example:

```text
bit 0 -> 1
bit 1 -> 2
...
bit 8 -> 9
```

This makes checking whether a number is already present a constant-time bit operation.

## Puzzle Generation and Uniqueness

After generating a complete Sudoku solution, cells are randomly removed.

For every candidate removal, the game temporarily removes the value and counts the number of possible solutions.

If the resulting puzzle has exactly one solution, the removal is kept. Otherwise, the original value is restored.

The solution counter stops once more than one solution is found, since the generator only needs to distinguish between:

```text
0/1 solution
multiple solutions
```

The relevant logic is implemented in `count_solutions()` and `remove_cells()`.

## Fixed Cell Tracking

Fixed cells are tracked separately from the Sudoku board.

Instead of maintaining another `int[9][9]` array, the project uses:

```c
typedef struct {
    u64 low;
    u64 high;
} FixedMask;
```

There are 81 Sudoku cells, so two 64-bit integers are sufficient:

```text
low  -> cells 0 - 63
high -> cells 64 - 80
```

A cell can then be checked with a bit operation instead of indexing another grid.

## Save / Load

The current game can be saved to:

```text
saved.dat
```

The saved data contains:

- Current puzzle
- Complete solution
- Fixed-cell mask
- Cursor position

The game state is serialized into a `SaveData`  in binary format.

## Terminal UI

The UI is implemented with ncurses windows.

The project separates the main UI into states:

```text
MENU
GAME
HELP
EXIT
```

Each state has its own window and input handling.

The game board uses configurable dimensions and cell spacing:

```c
#define W_WIDTH  46
#define W_HEIGHT 19

#define H_STEP 5
#define V_STEP 2
```

The grid supports wide characters through ncurses' wide-character functionality, allowing Unicode box-drawing characters to be used for custom line styles.

## Project Structure

The project is currently organized around the following responsibilities:

```text
.
├── source files
├── title1.txt
├── saved.dat          # generated at runtime
└── README.md
```

The current implementation keeps the game, UI, Sudoku generation, input handling, and save/load functionality in the C source rather than splitting them into separate modules.

## Building

The project requires:

- C compiler
- ncurses
- wide-character ncurses support
- A UTF-8 capable terminal

On Linux, install the ncurses development package appropriate for your distribution.

Because the program uses wide-character ncurses functionality, compile/link against `ncursesw`:

```bash
gcc -std=c11 -Wall -Wextra -o sudoku sudoku.c -lncursesw
```

Then run:

```bash
./sudoku
```

The program expects `title1.txt` to be available from the working directory when the title is loaded.

## Terminal Compatibility

The UI relies on terminal capabilities exposed through ncurses and on Unicode rendering provided by the terminal/font.

The game has been tested during development with terminal emulators such as:

- Kitty
- Alacritty

Visual differences may occur between terminal emulators, particularly for Unicode box-drawing characters and screen update behavior.

## Implementation Details

### Sudoku representation

The board is represented as:

```c
typedef i32 Sudoku[9][9];
```

A value of `0` represents an empty cell.

### Position

The current editable cell is represented by:

```c
typedef struct {
    i32 x;
    i32 y;
} Position;
```

The position stores Sudoku-relative coordinates rather than terminal coordinates.

Terminal coordinates are calculated from the cell position using the configured board offsets and spacing.

### UI State

The game context contains the current puzzle, solution, UI state, difficulty, menu state, fixed-cell mask, and running state.

This allows input handling and rendering to depend on the current application state without mixing menu and game logic.


No license has been specified for this project yet.
