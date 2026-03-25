#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <raylib.h>

#define HEIGHT 1000
#define WIDTH 1500

#define WIDTH_TILES 40
#define HEIGHT_TILES 25

#define TEXTURE_SCALE 2
#define TEXTURE_SIZE 16
#define TEXTURE_COUNT 13
#define TILES_OFFSET_X (WIDTH - TEXTURE_SIZE * TEXTURE_SCALE * WIDTH_TILES)/2.0
#define TILES_OFFSET_Y (HEIGHT - TEXTURE_SIZE * TEXTURE_SCALE * HEIGHT_TILES)/2.0

#define EASY_COUNT (int) (WIDTH_TILES * HEIGHT_TILES * 0.1)
#define MEDIUM_COUNT (int) (WIDTH_TILES * HEIGHT_TILES * 0.15)
#define HARD_COUNT (int) (WIDTH_TILES * HEIGHT_TILES * 0.2)

#define BUTTON_WAIT 15

typedef enum State {
	MENU,
	IDLE,
	GAME,
	END_WIN,
	END_LOSE
} State;

typedef enum Difficulty {
	EASY,
	MEDIUM,
	HARD
} Difficulty;

typedef enum TileType {
	N0,
	N1,
	N2,
	N3,
	N4,
	N5,
	N6,
	N7,
	N8,
	MINE,
	EXPLODED_MINE
} TileType;

typedef enum TileState {
	HIDDEN = 11,
	FLAGGED,
	DUG
} TileState;

typedef struct Tile {
	TileType type;
	TileState state;
} Tile;

typedef struct TileLoc {
	Tile* tile;
	Vector2 coord;
} TileLoc;

typedef struct Neighbors {
	TileLoc* tiles;
	int count;
} Neighbors;

typedef struct Button {
	char* text;
	int font_size;
	Vector2 pos;
	Vector2 size;
	bool pressed;	
	bool just_released;
	bool highlighted;
} Button;

#define MENU_BUTTONS_COUNT 5
typedef struct MenuButtons {
	Button play;
	Button easy;
	Button medium;
	Button hard;
	Button quit;
	Button* buttons[5];
	int button_count;
} MenuButtons;

#define END_BUTTONS_COUNT 2
typedef struct EndButtons {
	Button play_again;
	Button menu;
	Button* buttons[END_BUTTONS_COUNT];
	int button_count;
} EndButtons;


// FUNCTIONS
Vector2 Vec2(int x, int y);
Vector2 vec2_add(Vector2 a, Vector2 b);

void draw_text_centered(char* text, int x, int y, int font_size); 
void draw_text_background(char* text, Vector2 pos, Vector2 size, int font_size, Color color);

Button create_button(char* text, Vector2 pos, Vector2 size, int font_size);
void draw_button(Button button);
bool in_button(Button but, Vector2 pos);

#endif
