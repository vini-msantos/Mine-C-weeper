#include "definitions.h"

Vector2 Vec2(int x, int y) {
	return (Vector2) {x, y};
}

Vector2 vec2_add(Vector2 a, Vector2 b) {
	return (Vector2) {a.x + b.x, a.y + b.y };
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

Button create_button(char* text, Vector2 pos, Vector2 size, int font_size) {
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
