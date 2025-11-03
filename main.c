#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define CLEAR_SCREEN "cls"
#else
#define CLEAR_SCREEN "clear"
#endif

static const int BOARD_SIZE = 9;
static const int MINE_COUNT = 10;

static const char* const RESET = "\033[0m";
static const char* const BLACK = "\033[30m";
static const char* const RED = "\033[31m";
static const char* const GREEN = "\033[32m";
static const char* const YELLOW = "\033[33m";
static const char* const BLUE = "\033[34m";
static const char* const MAGENTA = "\033[35m";
static const char* const CYAN = "\033[36m";
static const char* const WHITE = "\033[37m";
static const char* const BOLD = "\033[1m";

static const char* const BG_RED = "\033[41m";
static const char* const BG_GREEN = "\033[42m";
static const char* const BG_YELLOW = "\033[43m";
static const char* const BG_MAGENTA = "\033[45m";
static const char* const BG_GRAY = "\033[100m";

typedef enum {
	GANE_PLAYING = 0,
	GAME_WON = 1,
	GAME_LOST = 2,

} GameState;

typedef struct {
	char board[9][9];
	bool revealed[9][9];
	bool flagged[9][9];
	int flagCount;
	int revealedCount;
	GameState state;
	int mineRow;
	int mineCol;
} GameBoard;

static void initalizeConsole(void);
static void initalizeGame(GameBoard* game);
static void placeMines(GameBoard* game);
static void calculateAdjacentMines(GameBoard* game);
static void displayBoard(const GameBoard* game);
static void displayFullMap(const GameBoard* game);
static void printCell(const GameBoard* game, int row, int col);
static void printFullMapCell(const GameBoard* game, int row, int col);
static bool revealCell(GameBoard* game, int row, int col);
static void revealAllCells(GameBoard* game, int row, int col);
static void toggleFlag(GameBoard* game, int row, int col);
static bool checkWinCondition(const GameBoard* game);
static void displayGameStatistics(const GameBoard* game);
static bool isValidCoordinate(int row, int col);
static void clearInputBuffer(void);
static void printColoredNumber(char number);
static void printBorder(bool isTop);
static void printGameHeader(void);
static void printGameInstructions(void);
static bool processPlayerAction(GameBoard* game);
static int safeInput(char* action, int* row, int* col);

int main(void) {
	initalizeConsole();
	srand((unsigned int)time(NULL));

	GameBoard game;
	initalizeGame(&game);

	printGameHeader();
	printGameInstructions();

	placeMines(&game);
	calculateAdjacentMines(&game);

	displayFullMap(&game);

	while (game.state == GANE_PLAYING) {
		displayBoard(&game);

		if (!processPlayerAction(&game)) {
			continue;
		}

		if (checkWinCondition(&game)) {
			game.state = GAME_WON;
		}
	}

	displayBoard(&game);
	displayFullMap(&game);

	if (game.state == GAME_WON) {
		printf("%s%s모든 지뢰를 찾음 %s\n", BG_GREEN, BOLD, RESET);
	}
	else {
		printf("%s%s지뢰를 밟음 %s\n", BG_RED, BOLD, RESET);
	}

	return 0;
}

static void initalizeConsole(void) {
#ifdef _WIN32
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE) {
		DWORD dwMode = 0;
		if (GetConsoleMode(hOut, &dwMode)) {
			dwMode != 0x0004;
			SetConsoleMode(hOut, dwMode);
		}
	}
#endif
}

static void initalizeGame(GameBoard* game) {
	if (game == NULL) return;

	memset(game->board, '0', sizeof(game->board));
	memset(game->revealed, false, sizeof(game->revealed));
	memset(game->flagged, false, sizeof(game->flagged));

	game->flagCount = 0;
	game->revealedCount = 0;
	game->state = GANE_PLAYING;
	game->mineRow = -1;
	game->mineCol = -1;
}

static void placeMines(GameBoard* game) {
	if (game == NULL) return;

	int minesPlaced = 0;

	while (minesPlaced < MINE_COUNT) {
		const int row = rand() % BOARD_SIZE;
		const int col = rand() % BOARD_SIZE;

		if (game->board[row][col] != '*') {
			game->board[row][col] = '*';
			minesPlaced++;
		}
	}
}

static void calculateAdjacentMines(GameBoard* game) {
	if (game == NULL) return;

	for (int row = 0; row < BOARD_SIZE; row++) {
		for (int col = 0; col < BOARD_SIZE; col++) {
			if (game->board[row][col] == '*') {
				continue;
			}

			int mineCount = 0;

			for (int dRow = -1; dRow <= 1; dRow++) {
				for (int dCol = -1; dCol <= 1; dCol++) {
					if (dRow == 0 && dCol == 0) continue;

					const int newRow = row + dRow;
					const int newCol = col + dCol;

					if (isValidCoordinate(newRow, newCol) && game->board[newRow][newCol] == '*') {
						mineCount++;
					}
				}
			}
			game->board[row][col] = '0' + (char)mineCount;
		}
	}
}

static void displayBoard(const GameBoard* game) {
	if (game == NULL) return;

	printf("\n   ");
	for (int col = 0; col < BOARD_SIZE; col++) {
		printf("%s%2d %s", CYAN, col, RESET);
	}
	printf("\n");

	printBorder(true);

	for (int row = 0; row < BOARD_SIZE; row++) {
		printf("%s%2d %s", CYAN, row, RESET);
		for (int col = 0; col < BOARD_SIZE; col++) {
			printCell(game, row, col);
		}
		printf("\n");
	}

	printBorder(false);

	printf("%s[FLAGS: %d / MINES: %d]%s\n",	YELLOW, game->flagCount, MINE_COUNT, RESET);
}

static void printCell(const GameBoard* game, int row, int col) {
	if (game == NULL || !isValidCoordinate(row, col)) return;

	if (game->flagged[row][col]) {
		printf("%s%s F %s", BG_YELLOW, RED, RESET);
		return;
	}

	if (!game->revealed[row][col]) {
		printf("%s   %s", BG_GRAY, RESET);
		return;
	}

	const char cell = game->board[row][col];

	if (cell == '*') {
		printf("%s%s * %s", BG_RED, BOLD, RESET);
		return;
	}

	else {
		printColoredNumber(cell);
	}
}

static void printColoredNumber(char number) {
	switch (number) {
	case '1': printf("%s%s %c %s", BOLD, BLUE, number, RESET); break;
	case '2': printf("%s%s %c %s", BOLD, GREEN, number, RESET); break;
	case '3': printf("%s%s %c %s", BOLD, RED, number, RESET); break;
	case '4': printf("%s%s %c %s", BOLD, MAGENTA, number, RESET); break;
	case '5': printf("%s%s %c %s", BOLD, YELLOW, number, RESET); break;
	case '6': printf("%s%s %c %s", BOLD, CYAN, number, RESET); break;
	case '7': printf("%s%s %c %s", BOLD, BLACK, number, RESET); break;
	case '8': printf("%s%s %c %s", BOLD, WHITE, number, RESET); break;
	default: printf(" %c ", number); break;
	}
}

static bool revealColl(GameBoard* game, int row, int col) {
	if (game == NULL || !isValidCoordinate(row, col)) {
		return true;
	}

	if (game->revealed[row][col] || game->flagged[row][col]) {
		return true;
	}

	if (game->board[row][col] == '*') {
		game->mineRow = row;
		game->mineCol = col;
		game->state = GAME_LOST;
		game->revealed[row][col] = true;
		return false;
	}

	game->revealed[row][col] = true;
	game->revealedCount++;

	if (game->board[row][col] == '0') {
		revealAllCells(game, row, col);
	}

	return true;
}

static void revealAllCells(GameBoard* game, int row, int col) {
	for (int dRow = -1; dRow <= 1; dRow++) {
		for (int dCol = -1; dCol <= 1; dCol++) {
			if (dRow == 0 && dCol == 0) continue;

			const int newRow = row + dRow;
			const int newCol = col + dCol;

			if (isValidCoordinate(newRow, newCol)) {
				revealCell(game, newRow, newCol);
			}
		}
	}
}

static void toggleFlag(GameBoard* game, int row, int col) {
	if (game == NULL || !isValidCoordinate(row, col)) return;

	if (game->revealed[row][col]) {
		printf("%s[WARNING] Cannot flag revealed cell%s\n", YELLOW, RESET);
		return;
	}

	game->flagged[row][col] = !game->flagged[row][col];

	if (game->flagged[row][col]) {
		game->flagCount++;
		printf("%s[FLAG] flag placed at %d, %d%s\n", GREEN, row, col, RESET);
	}
	else {
		game->flagCount--;
		printf("%s[FLAG] flag removed from %d, %d%s\n", YELLOW, row, col, RESET);
	}
}

static bool checkWinCondition(const GameBoard* game) {
	if (game == NULL) return false;

	const int totalCells = BOARD_SIZE * BOARD_SIZE;
	const int safeCells = totalCells - MINE_COUNT;

	return (game->revealedCount == safeCells);
}

static bool isValidCoordinate(int row, int col) {
	return (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE);
}

static void clearInputBuffer(void) {
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
}

static void printBorder(bool isTop) {
	printf("   +");
	for (int i = 0; i < BOARD_SIZE; i++) {
		printf("---");
	}
	printf("--\n");
}

static void printGameHeader(void) {
	printf("\n%s%s", BOLD, CYAN);
	printf(" +==========================+");
	printf(" |        MINESWEEPER       |");
	printf(" +==========================+");
	printf("%s", RESET);
	printf("%sBoard: %dx%d | Mines: %d%s\n", YELLOW, BOARD_SIZE, BOARD_SIZE, MINE_COUNT, RESET);
}

static void printGameInstructions(void) {
	printf("%s", WHITE);
	printf(" ------------------------------\n");
	printf("%s%sInstructions : %s\n", GREEN, BOLD, RESET);
	printf("%s", GREEN);
	printf(" Open Cell : o [row] [col]\n");
	printf(" Toggle Flag : f [row] [col]\n");
	printf(" Coordinates: 0-%d\n", BOARD_SIZE - 1);
	printf("%s", WHITE);
	printf(" ------------------------------\n");
	printf("%s", RESET);
}

static bool revealCell(GameBoard* game, int row, int col) {
	if (game == NULL || !isValidCoordinate(row, col)) {
		return true;
	}

	if (game->revealed[row][col] || game->flagged[row][col]) {
		return true;
	}

	if (game->board[row][col] == '*') {
		game->mineRow = row;
		game->mineCol = col;
		game->state = GAME_LOST;
		game->revealed[row][col] = true;
		return false;
	}

	game->revealed[row][col] = true;
	game->revealedCount++;

	if (game->board[row][col] == '0') {
		revealAllCells(game, row, col);
	}

	return true;
}

static int safeInput(char* action, int* row, int* col) {
#ifdef _MSC_VER
	return scanf_s(" %c %d %d", action, 1, row, col);
#else
	return scanf(" %c %d %d", action, row, col);
#endif
}

static bool processPlayerAction(GameBoard* game) {
	char action;
	int row, col;

	printf("\n%sCommand (o/f row col): %s", CYAN, RESET);

	if (safeInput(&action, &row, &col) != 3) {
		clearInputBuffer();
		printf("%s[ERROR] Invalid input format%s\n", RED, RESET);
		return false;
	}

	clearInputBuffer();

	if (!isValidCoordinate(row, col)) {
		printf("%s[ERROR] Invalid Coordinates (0-%d)%s\n", RED, BOARD_SIZE - 1, RESET);
		return false;
	}

	switch (action) {
	case 'o':
	case 'O':
		if (game->flagged[row][col]) {
			printf("%s[WARNING] Remove flag first%s\n", YELLOW, RESET);
			return false;
		}
		if (game->revealed[row][col]) {
			printf("%s[WARNING] Cell already revealed%s\n", YELLOW, RESET);
			return false;
		}
		return revealCell(game, row, col);
	case 'f':
	case 'F':
		toggleFlag(game, row, col);
		return false;

	default:
		printf("%s[ERROR] Unknown command '%c'%s\n", RED, action, RESET);
		return false;
	}
}
static void displayFullMap(const GameBoard* game) {
	if (game == NULL) return;

	printf("\n%s%s======== FULL MAP REVEAL ========%s\n", NULL, CYAN, RESET);
	printf("%sLegend:%s\n", YELLOW, RESET);
	printf(" * = Mine\n");
	printf(" X = Triggered mine\n");
	printf(" V = Correct flag\n");
	printf(" W = Wrong flag\n");
	printf(" Numbers = Adjust mine count\n");

	printf("\n    ");
	for (int col = 0; col < BOARD_SIZE; col++) {
		printf("%s%2d %s", CYAN, col, RESET);
	}
	printf("\n");

	printBorder(true);

	for (int row = 0; row < BOARD_SIZE; row++) {
		printf("%s%2d %s", CYAN, row, RESET);
		for (int col = 0; col < BOARD_SIZE; col++) {
			printFullMapCell(game, row, col);
		}
		printf("\n");
	}
	
	printBorder(false);

	displayGameStatistics(game);
}

static void printFullMapCell(const GameBoard* game, int row, int col) {
	if (game == NULL || !isValidCoordinate(row, col)) return;

	const char cell = game->board[row][col];
	const bool isMine = (cell == '*');
	const bool isFlagged = game->flagged[row][col];

	if (isMine && row == game->mineRow && col == game->mineCol) {
		printf("%s%s X %s", BG_RED, BOLD, RESET);
	}

	else if (isMine && isFlagged) {
		printf("%s%s V %s", BG_GREEN, BOLD, RESET);
	}

	else if (!isMine && isFlagged) {
		printf("%s%s W %s", BG_RED, BOLD, RESET);
	}
	else if (isMine && !isFlagged) {
		printf("%s%s * %s", BG_MAGENTA, BOLD, RESET);
	}

	else if (cell == '0') {
		printf("  ");
	}

	else {
		printColoredNumber(cell);
	}
}

static void displayGameStatistics(const GameBoard* game) {
	if (game == NULL) return;

	int correctFlags = 0;
	int wrongFlags = 0;

	for (int row = 0; row < BOARD_SIZE; row++) {
		for (int col = 0; col < BOARD_SIZE; col++) {
			if (game->flagged[row][col]) {
				correctFlags++;
			}
			else {
				wrongFlags++;
			}
		}
	}
	printf("\n%s%s게임 통계: %s\n", BOLD, WHITE, RESET);
	printf("%s 깃발: %d/%d%s\n", GREEN, correctFlags, MINE_COUNT, RESET);
	printf("%s 잘못된 깃발: %d%s\n", RED, wrongFlags, RESET);
	printf("%s 총 깃발 수: %d%s\n", YELLOW, game->flagCount, RESET);

	float detectionRate = 0.0f;
	if (MINE_COUNT > 0) {
		detectionRate = (float)correctFlags / MINE_COUNT * 100;
	}

	printf("%s 지뢰 발견 확률: %.1f%%%s\n", CYAN, detectionRate, RESET);
}
