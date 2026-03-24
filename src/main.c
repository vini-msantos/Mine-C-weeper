#include <raylib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

MenuButtons menu_buttons;
EndButtons end_buttons;

Texture textures[TEXTURE_COUNT];
Texture flag_texture;
Difficulty difficulty = MEDIUM;
State game_state = MENU;
bool game_should_exit = false;

Tile board[WIDTH_TILES][HEIGHT_TILES];

int flag_count = 0;
int tiles_dug = 0;
int mine_count = 0;

int frames_since_last_state = 0;

Vector2 Vec2(int x, int y) {
	return (Vector2) {x, y};
}

void draw_text_centered(char* text, int x, int y, int font_size) {
	int text_size_x = MeasureText(text, font_size);
	int pos_x = x - text_size_x/2;
	int pos_y = y - font_size/2;
	DrawText(text, pos_x, pos_y, font_size, WHITE);
}

void draw_text_background(char* text, Vector2 pos, Vector2 size, int font_size, Color color) {
	Rectangle rect = {
		.x = pos.x - size.x/2,
		.y = pos.y - size.y/2,
		.width = size.x,
		.height = size.y
	};
	DrawRectangleRounded(rect, 0.5f, 0, color);
	draw_text_centered(text, pos.x, pos.y, font_size);
}

int get_difficulty_mine_count(void) {
	switch (difficulty) {
		case EASY: return EASY_COUNT;
		case MEDIUM: return MEDIUM_COUNT;
		case HARD: return HARD_COUNT;
	}
}


void neighbors_free(Neighbors neighbors) {
	free(neighbors.tiles);
}

Neighbors get_neighbors(int x, int y) {
	Neighbors neighbors = {
		.tiles = (TileLoc*) calloc(8, sizeof(TileLoc))
	};
	int count = 0;

	for (int xi = x-1; xi <= x+1; xi++) {
		for (int yi = y-1; yi <= y+1; yi++) {
			if (xi < 0 || xi >= WIDTH_TILES || yi < 0 || yi >= HEIGHT_TILES) continue;
			if (xi == x && yi == y) continue;
			
			neighbors.tiles[count] = (TileLoc) {
				.tile = &board[xi][yi],
				.coord = (Vector2) {xi, yi}
			};
			count++;
		}
	}
	
	neighbors.count = count;
	return neighbors;
}

void increment_neighbors(int x, int y) {
	Neighbors nbrs = get_neighbors(x, y);
	for (int i = 0; i < nbrs.count; i++) {
		Tile* nbr = nbrs.tiles[i].tile;
		if (nbr->type < MINE) {
			nbr->type++;
		}
	}
	neighbors_free(nbrs);
}

bool can_place_mine(int x, int y, int start_x, int start_y) {
	bool dx = (start_x - x) > 2 || (start_x - x) < -2;
	bool dy = (start_y - y) > 2 || (start_y - y) < -2;

	return board[x][y].type != MINE && (dx || dy);
}

void place_mines(int mines, int start_x, int start_y) {
	int count = 0;

	while (count < mines) {
		int x = GetRandomValue(0, WIDTH_TILES-1);
		int y = GetRandomValue(0, HEIGHT_TILES-1);

		if (!can_place_mine(x, y, start_x, start_y)) continue;

		board[x][y].type = MINE;
		increment_neighbors(x, y);
		count++;
	}
	mine_count = count;
}

Texture* get_tile_texture(Tile tile) {
	if (tile.state != DUG) return &textures[tile.state];

	return &textures[tile.type];
}

Vector2 vec2_add(Vector2 a, Vector2 b) {
	return (Vector2) {a.x + b.x, a.y + b.y };
}

void draw_flag_count(void) {
	Vector2 offset = {30, 30};
	DrawTextureEx(flag_texture, offset, 0, 2, WHITE);
	DrawText(TextFormat("%d", mine_count - flag_count), offset.x + 36, offset.y + TEXTURE_SIZE/2.0, 24, WHITE);
}

void draw_tile(Tile tile, int x, int y) {
	Texture* texture = get_tile_texture(tile);
	Vector2 pos = {
		.x = TEXTURE_SCALE * TEXTURE_SIZE * x,
		.y = TEXTURE_SCALE * TEXTURE_SIZE * y
	};
	Vector2 offset = {
		.x = TILES_OFFSET_X,
		.y = TILES_OFFSET_Y
	};

	DrawTextureEx(*texture, vec2_add(pos, offset), 0, TEXTURE_SCALE, WHITE);
}

void draw_board(void) {
	for (int x = 0; x < WIDTH_TILES; x++) {
		for (int y = 0; y < HEIGHT_TILES; y++) {
			draw_tile(board[x][y], x, y);
		}
	}
}

Button create_button(char* text, Vector2 pos, Vector2 size, int font_size ) {
	return (Button) {
		.text = text,
		.pos = pos,
		.size = size,
		.font_size = font_size,
		.pressed = false,
		.just_released = false,
		.highlighted = false
	};
}

void create_menu_buttons() {
	int x = WIDTH / 2;
	int font_size = 40;
	Vector2 size = {180, 60};

	menu_buttons.play = create_button("Play", Vec2(x, HEIGHT*0.55), size, font_size);
	menu_buttons.buttons[0] = &menu_buttons.play;

	menu_buttons.quit = create_button("Quit", Vec2(x, HEIGHT*0.90), size, font_size); 
	menu_buttons.buttons[1] = &menu_buttons.quit;

	menu_buttons.easy = create_button("Easy", Vec2(x - (size.x + 30), HEIGHT * 0.65), size, font_size);
	menu_buttons.buttons[2] = &menu_buttons.easy;

	menu_buttons.medium = create_button("Medium", Vec2(x, HEIGHT * 0.65), size, font_size);
	menu_buttons.buttons[3] = &menu_buttons.medium;

	menu_buttons.hard = create_button("Hard", Vec2(x + (size.x + 30), HEIGHT * 0.65), size, font_size);
	menu_buttons.buttons[4] = &menu_buttons.hard;

	menu_buttons.button_count = MENU_BUTTONS_COUNT;
}

void create_end_buttons(void) {
	int x = WIDTH / 2;
	int font_size = 40;
	Vector2 size = {240, 60};

	end_buttons.play_again = create_button("Play Again", Vec2(x, HEIGHT*0.45), size, font_size);
	end_buttons.buttons[0] = &end_buttons.play_again;

	end_buttons.menu = create_button("Menu", Vec2(x, HEIGHT*0.55), size, font_size); 
	end_buttons.buttons[1] = &end_buttons.menu;

	end_buttons.button_count = END_BUTTONS_COUNT;
}

void draw_button(Button button) {
	Color color = {50, 50, 50, 255};
	if (button.highlighted) {
		color = (Color){80, 80, 80, 255};
	}
	draw_text_background(button.text, button.pos, button.size, button.font_size, color);
}

bool in_button(Button but, Vector2 pos) {
	bool inside_x = (pos.x >= but.pos.x - but.size.x/2) && (pos.x <= but.pos.x + but.size.x/2);
	bool inside_y = (pos.y >= but.pos.y - but.size.y/2) && (pos.y <= but.pos.y + but.size.y/2);
	return inside_x && inside_y;
}

void update_buttons(Button* buttons[], int button_count) {
	bool mouse_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
	Vector2 pos = GetMousePosition();
	
	for (int i = 0; i < button_count; i++) {
		Button* button = buttons[i];

		bool mouse_in = in_button(*button, pos);
		button->highlighted = mouse_in;

		if (mouse_in && frames_since_last_state >= BUTTON_WAIT) {
			button->just_released = mouse_released;
		}
	}
}

// ENTER STATE LOGIC
void enter_menu_state(void) {
	game_state = MENU;
	frames_since_last_state = 0;
	create_menu_buttons();
}

void enter_idle_state(void) {
	game_state = IDLE;
	flag_count = 0;
	tiles_dug = 0;
	frames_since_last_state = 0;

	for (int y = 0; y < HEIGHT_TILES; y++) {
		for (int x = 0; x < WIDTH_TILES; x++) {
			board[x][y] = (Tile) {
				.state = HIDDEN,
				.type = N0, 
			};	
		}
	}
}

void enter_game_state(int start_x, int start_y) {
	game_state = GAME;
	frames_since_last_state = 0;
	place_mines(get_difficulty_mine_count(), start_x, start_y);
}

void enter_end_state(bool has_won) {
	game_state = END_LOSE;
	if (has_won) game_state = END_WIN;
	
	frames_since_last_state = 0;
	create_end_buttons();
	for (int x = 0; x < WIDTH_TILES; x++) {
		for (int y = 0; y < HEIGHT_TILES; y++) {
			Tile* tile = &board[x][y];
			
			if (tile->state == FLAGGED && tile->type < MINE) {
				tile->state = HIDDEN;
			}
			
			if (tile->state == HIDDEN && tile->type >= MINE) {
				tile->state = DUG;
			}
		}
	}
}

void check_win(void) {
	if (flag_count + tiles_dug == WIDTH_TILES * HEIGHT_TILES && game_state == GAME) {
		enter_end_state(true);
	}
}

void handle_lclick_board(int x, int y, bool recursive);

void dig_tile(int x, int y) {
	Tile* tile = &board[x][y];

	if (tile->state != DUG) tiles_dug++;
	tile->state = DUG;
	
	if (game_state == IDLE) {
		enter_game_state(x, y);
	}
	
	if (tile->type == MINE) {
		tile->type = EXPLODED_MINE;
		enter_end_state(false);
	}

	if (tile->type == N0) {
		Neighbors nbrs = get_neighbors(x, y);
		for (int i = 0; i < nbrs.count; i++) {
			Vector2 pos = nbrs.tiles[i].coord;
			handle_lclick_board(pos.x, pos.y, true);
		}
		neighbors_free(nbrs);
	}

	check_win();
}

int neighbors_flag_count(Neighbors nbrs) {
	int count = 0;
	for (int i = 0; i < nbrs.count; i++) {
		if (nbrs.tiles[i].tile->state == FLAGGED) count++;
	}
	return count;
}

void handle_lclick_board(int x, int y, bool recursive) {
	Tile* tile = &board[x][y];

	if (tile->state == FLAGGED) return;
	if (tile->state == DUG && tile->type == N0) return;
	if (tile->state == HIDDEN) return dig_tile(x, y);
	if (tile->state == DUG && tile->type < MINE && !recursive) {
		Neighbors nbrs = get_neighbors(x, y);
		if (neighbors_flag_count(nbrs) == tile->type) {
			for (int i = 0; i < nbrs.count; i++) {
				Vector2 pos = nbrs.tiles[i].coord;
				handle_lclick_board(pos.x, pos.y, true);
			}
		}
		neighbors_free(nbrs);
	}
}

void handle_rclick_board(int x, int y) {
	Tile* tile = &board[x][y];

	switch (tile->state) {
		case HIDDEN:
			tile->state = FLAGGED;
			flag_count++;
			break;
		case FLAGGED:
			tile->state = HIDDEN;
			flag_count--;
			break;
		default: break;
	}

	check_win();
}

void check_board_input(void) {
	bool lmb = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_D);
	bool rmb = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_F);
	if (!lmb && !rmb) return;
		
	Vector2 pos = GetMousePosition();
	int x = (pos.x - TILES_OFFSET_X) / (TEXTURE_SCALE * TEXTURE_SIZE);
	int y = (pos.y - TILES_OFFSET_Y) / (TEXTURE_SCALE * TEXTURE_SIZE);

	if (x < 0 || y < 0 || x >= WIDTH_TILES || y >= HEIGHT_TILES) return;
	
	if (lmb) return handle_lclick_board(x, y, false);
	if (rmb) return handle_rclick_board(x, y);
}

// UI Logic
void play_button_pressed(void) {
	enter_idle_state();
}

void quit_button_pressed(void) {
	game_should_exit = true;
}


// UPDATE STATE LOGIC
void update_menu_state(void) {
	update_buttons(menu_buttons.buttons, menu_buttons.button_count);

	menu_buttons.easy.highlighted = difficulty == EASY;
	menu_buttons.medium.highlighted = difficulty == MEDIUM;
	menu_buttons.hard.highlighted = difficulty == HARD;

	if (menu_buttons.easy.just_released) difficulty = EASY;
	if (menu_buttons.medium.just_released) difficulty = MEDIUM;
	if (menu_buttons.hard.just_released) difficulty = HARD;
	if (menu_buttons.play.just_released) return play_button_pressed();
	if (menu_buttons.quit.just_released) return quit_button_pressed();
}

void update_idle_state(void) {
	check_board_input();
}

void update_game_state(void) {
	check_board_input();
}

void update_end_state(void) {
	update_buttons(end_buttons.buttons, end_buttons.button_count);

	if (end_buttons.play_again.just_released) return play_button_pressed();
	if (end_buttons.menu.just_released) return enter_menu_state();
}

void draw_menu(void) {
	for (int i = 0; i < menu_buttons.button_count; i++) {
		draw_button(*menu_buttons.buttons[i]);
	}

	draw_text_background("Mine-C-weeper", Vec2(WIDTH/2, HEIGHT*0.3), Vec2(540, 90), 60, (Color){50,50,50,255});
}

void draw_end(bool has_won) {
	for (int i = 0; i < end_buttons.button_count; i++) {
		draw_button(*end_buttons.buttons[i]);
	}

	char* text = "You Lose!";
	if (has_won) text = "You Win!";
	
	draw_text_background(text, Vec2(WIDTH/2, HEIGHT*0.3), Vec2(340, 90), 60, (Color){50,50,50,255});
}

void load_textures(void) {
	for (int i = 0; i < TEXTURE_COUNT; i++) {
		textures[i] = LoadTexture(TextFormat("./src/Sprites/%d.png", i));
	}	
	flag_texture = LoadTexture("./src/Sprites/flag.png");
}

void unload_textures(void) {
	for (int i = 0; i < TEXTURE_COUNT; i++) {
		UnloadTexture(textures[i]);
	}
	UnloadTexture(flag_texture);
}

void handle_update(void) {
	frames_since_last_state += 1;
	switch (game_state) {
		case MENU: return update_menu_state();
		case IDLE: return update_idle_state();
		case GAME: return update_game_state();
		case END_WIN:
		case END_LOSE:  return update_end_state();
	}
}

void handle_drawing(void) {
	BeginDrawing();

	ClearBackground(SKYBLUE);	

	switch (game_state) {
		case MENU:
			draw_menu();
			break;
		
		case IDLE:
			draw_board();
			break;

		case GAME:
			draw_flag_count();
			draw_board();
			break;

		case END_WIN:
			draw_board();
			draw_end(true);
			break;
		case END_LOSE: 
			// TODO
			draw_board();
			draw_end(false);
			break;

		default: break; // TODO
	}

	EndDrawing();
}

int main() {
	InitWindow(WIDTH, HEIGHT, "cuca beludo games");
	
	SetTargetFPS(60);

	load_textures();

	enter_menu_state();
	
	while (!WindowShouldClose() && !game_should_exit) {
		handle_update();

		handle_drawing();		
	}

	unload_textures();
	
	CloseWindow();
	
	return 0;
}
