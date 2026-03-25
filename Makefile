default:
	gcc src/main.c src/definitions.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -Llib -Iinclude -o minecweeper

