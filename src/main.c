#include <raylib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "definitions.h"

// SECTION: GLOBAL VARIABLES
// UI ELEMENTS
MenuButtons menu_buttons;
EndButtons end_buttons;

// TEXTURES
Texture textures[TEXTURE_COUNT];
Texture flag_texture;

// GAME STATES
State game_state = MENU;
int frames_since_last_state = 0;
bool game_should_exit = false;

Difficulty difficulty = MEDIUM;
int flag_count = 0;
int tiles_dug = 0;
int mine_count = 0;

Tile board[WIDTH_TILES][HEIGHT_TILES];
// END_SECTION: GLOBAL VARIABLES


// SECTION: FUNCTION PROTOTYPES
// HANDLING NEIGHBORING TILES
Neighbors get_neighbors(int x, int y);
void neighbors_free(Neighbors neighbors);
int neighbors_flag_count(Neighbors nbrs);
void increment_neighbors(int x, int y);

// PLACING MINES
bool can_place_mine(int x, int y, int start_x, int start_y);
int get_difficulty_mine_count(void);
void place_mines(int mines, int start_x, int start_y);

// HANDLING TEXTURES
void load_textures(void);
Texture* get_tile_texture(Tile tile);
void unload_textures(void);

// DRAWING
void draw_flag_count(void);
void draw_tile(Tile tile, int x, int y);

void handle_drawing(void);
void draw_menu(void);
void draw_board(void);
void draw_end(bool has_won);

// UI LOGIC
void create_menu_buttons(void);
void create_end_buttons(void);
void update_buttons(Button* buttons[], int button_count);
void play_button_pressed(void);
void quit_button_pressed(void);

// GAME LOGIC
void check_board_input(void);
void handle_lclick_board(int x, int y, bool recursive);
void handle_rclick_board(int x, int y);
void dig_tile(int x, int y);
void check_win(void);

// STATE LOGIC
void enter_menu_state(void);
void enter_idle_state(void);
void enter_game_state(int start_x, int start_y);
void enter_end_state(bool has_won);

void handle_update(void);
void update_menu_state(void);
void update_idle_state(void);
void update_game_state(void);
void update_end_state(void);
// END_SECTION: FUNCTION PROTOTYPES

// SECTION: DEFINITIONS
int get_difficulty_mine_count(void) {
	switch (difficulty) {
		case EASY: return EASY_COUNT;
		case MEDIUM: return MEDIUM_COUNT;
		case HARD: return HARD_COUNT;
	}
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

void neighbors_free(Neighbors neighbors) {
	free(neighbors.tiles);
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

int neighbors_flag_count(Neighbors nbrs) {
	int count = 0;
	for (int i = 0; i < nbrs.count; i++) {
		if (nbrs.tiles[i].tile->state == FLAGGED) count++;
	}
	return count;
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

Texture* get_tile_texture(Tile tile) {
	if (tile.state != DUG) return &textures[tile.state];

	return &textures[tile.type];
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

void handle_drawing(void) {
	BeginDrawing();

	ClearBackground((Color) {196, 240, 249});	

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
			draw_board();
			draw_end(false);
			break;
	}

	EndDrawing();
}

void create_menu_buttons(void) {
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

void play_button_pressed(void) {
	enter_idle_state();
}

void quit_button_pressed(void) {
	game_should_exit = true;
}

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
// END_SECTION: DEFINITIONS


int main() {
	InitWindow(WIDTH, HEIGHT, "cuca beludo games");
	
	SetTargetFPS(60);

	load_textures();
	SetExitKey(KEY_F12);

	enter_menu_state();
	
	while (!WindowShouldClose() && !game_should_exit) {
		if (IsKeyPressed(KEY_ESCAPE)) enter_menu_state();
		handle_update();

		handle_drawing();		
	}

	unload_textures();
	
	CloseWindow();
	
	return 0;
}
