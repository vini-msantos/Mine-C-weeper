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


typedef enum State {
	MENU,
	IDLE,
	GAME,
	END
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

Texture textures[TEXTURE_COUNT];
Difficulty difficulty = HARD;
State game_state = MENU;

Tile board[WIDTH_TILES][HEIGHT_TILES];

int flag_count = 0;

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

void place_mines(int start_x, int start_y) {
	int mine_count = 0;

	while (mine_count < get_difficulty_mine_count()) {
		int x = GetRandomValue(0, WIDTH_TILES-1);
		int y = GetRandomValue(0, HEIGHT_TILES-1);

		if (board[x][y].type == MINE || (x == start_x && y == start_y)) continue;

		board[x][y].type = MINE;
		increment_neighbors(x, y);
		mine_count++;
	}
}

Texture* get_tile_texture(Tile tile) {
	if (tile.state != DUG) return &textures[tile.state];

	return &textures[tile.type];
}

Vector2 vec2_add(Vector2 a, Vector2 b) {
	return (Vector2) {a.x + b.x, a.y + b.y };
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

// ENTER STATE LOGIC
void enter_menu_state(void) {
	game_state = MENU;
	// TODO
}

void enter_idle_state(void) {
	game_state = IDLE;

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
	place_mines(start_x, start_y);
}

void enter_end_state(void) {
	game_state = END;
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

void handle_lclick_board(int x, int y, bool recursive);

void dig_tile(int x, int y) {
	Tile* tile = &board[x][y];
	tile->state = DUG;
	
	if (game_state == IDLE) {
		enter_game_state(x, y);
	}
	
	if (tile->type == MINE) {
		tile->type = EXPLODED_MINE;
		enter_end_state();
	}

	if (tile->type == N0) {
		Neighbors nbrs = get_neighbors(x, y);
		for (int i = 0; i < nbrs.count; i++) {
			Vector2 pos = nbrs.tiles[i].coord;
			handle_lclick_board(pos.x, pos.y, true);
		}
		neighbors_free(nbrs);
	}
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
}

void check_board_input(void) {
	bool lmb = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
	bool rmb = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
	if (!lmb && !rmb) return;
		
	Vector2 pos = GetMousePosition();
	int x = (pos.x - TILES_OFFSET_X) / (TEXTURE_SCALE * TEXTURE_SIZE);
	int y = (pos.y - TILES_OFFSET_Y) / (TEXTURE_SCALE * TEXTURE_SIZE);

	if (x < 0 || y < 0 || x >= WIDTH_TILES || y >= HEIGHT_TILES) return;
	
	if (lmb) return handle_lclick_board(x, y, false);
	if (rmb) return handle_rclick_board(x, y);
}

// UPDATE STATE LOGIC
void update_menu_state(void) {
	// TODO
}

void update_idle_state(void) {
	check_board_input();
}

void update_game_state(void) {
	check_board_input();
}

void update_end_state(void) {
	// TODO
}

void load_textures(void) {
	for (int i = 0; i < TEXTURE_COUNT; i++) {
		textures[i] = LoadTexture(TextFormat("./Sprites/%d.png", i));
	}	
}

void unload_textures(void) {
	for (int i = 0; i < TEXTURE_COUNT; i++) {
		UnloadTexture(textures[i]);
	}
}

void handle_update(void) {
	switch (game_state) {
		case MENU: return update_menu_state();
		case IDLE: return update_idle_state();
		case GAME: return update_game_state();
		case END:  return update_end_state();
	}
}

void handle_drawing(void) {
	BeginDrawing();

	ClearBackground(SKYBLUE);	

	switch (game_state) {
		case IDLE:
		case GAME: draw_board(); break;
		case END: 
			// TODO
			draw_board();
			break;

		default: break; // TODO
	}

	EndDrawing();
}

int main() {
	InitWindow(WIDTH, HEIGHT, "cuca beludo games");
	
	SetTargetFPS(60);

	load_textures();

	enter_idle_state();
	
	while (!WindowShouldClose()) {
		handle_update();

		handle_drawing();		
	}

	unload_textures();
	
	CloseWindow();
	
	return 0;
}
