#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*
/////////////////////////////////////

Code for handling .tic files (see .tic formatspecification
here: https://github.com/nesbox/TIC-80/wiki/tic-File-Format)

You can inject external .lua files, as well as .gif files
for the cover. You can also export the current ones present
inside the cartridge.

If external files are greater than 65536 bytes (65KB, 2^16B),
then the program will simply reject them (impossible to insert
them into the cart, due to 16-bit limitations).

Certain .gif files may not work well when in SURF mode, but
they do work for carts shared in https://tic.computer/play

See printf below for help on how this program works (no scanfs
in the code, all filenames passed as args)

/////////////////////////////////////
*/

// Chunk representation struct
struct chunk_s {
	uint8_t bank_number, chunk_type;
	uint16_t chunk_size;
	uint8_t *data;
	struct chunk_s *next;
};
typedef struct chunk_s chunk_t;

// Colour representation struct
struct colour_s {
	uint8_t r, g, b;
};
typedef struct colour_s colour_t;

int main(int argc, const char **argv) {
	if (argc != 2 && argc < 4) { // Check argc
		printf("tic-cart <cart.tic> [-get-code | -set-code <code.lua>] | [-get-cover | -set-cover <cover.gif>]\n");
		return 0;
	}

	// Load file
	const char *cart_name = argv[1];
	FILE *cart;
	if (cart_name) {
		cart = fopen(cart_name, "rb");
		if (!cart) {
			printf("Error! Cart doesn't exist (cart loading error).\n");
			return 0;
		}
	}
	else {
		printf("Error! Cart not provided.\n");
		return 0;
	}
	printf("Loaded cart \"%s\" successfully.\n", cart_name);

	// Read from file and locate specific chunks
	chunk_t *file_data = NULL;
	chunk_t *c_code, *c_palette, *c_cover = NULL, *c_sprites = NULL, *c_tiles = NULL;
	while (1) {
		uint8_t chunk_info;
		chunk_t *new_chunk;
		
		// If failed to read, then it has probably reached EOF
		if (fread(&chunk_info, 1, 1, cart) < 1) break;

		// Init new chunk of data (.tic format specific)
		new_chunk = (chunk_t*) malloc(sizeof(chunk_t));

		// Read size
		fread(&new_chunk->chunk_size, 2, 1, cart);
		fseek(cart, 1, SEEK_CUR);
		
		// Get header info
		new_chunk->bank_number = chunk_info >> 5;
		new_chunk->chunk_type = chunk_info & 0x1F;
		
		// Get data section
		new_chunk->data = (uint8_t*) malloc(new_chunk->chunk_size);
		fread(new_chunk->data, 1, new_chunk->chunk_size, cart);

		// Add to linked list
		new_chunk->next = file_data;
		file_data = new_chunk;
		
		// Print
		const char *type;
		switch (new_chunk->chunk_type) {
			case 1:
			type = "Tiles";
			c_tiles = new_chunk;
			break;
			case 2:
			type = "Sprites";
			c_sprites = new_chunk;
			break;
			case 3:
			type = "Cover";
			c_cover = new_chunk;
			break;
			case 5:
			type = "Code";
			c_code = new_chunk;
			break;
			case 12:
			type = "Palette";
			c_palette = new_chunk;
			break;
			default:
			type = "Irrelevant";
			break;
		}
		printf(
			"Bank %d\nChunk type: %d (%s)\nChunk size (in bytes) is %d\n",
			new_chunk->bank_number, new_chunk->chunk_type, type, new_chunk->chunk_size
		);
	}
	fclose(cart);

	// Palette
	colour_t palette[16];
	memcpy(&palette, c_palette->data, 48);

	printf("Palette:\n");
	for (int i = 0; i < 16; i++) {
		printf("\e[48;2;%d;%d;%dm  \e[0m ", palette[i].r, palette[i].g, palette[i].b);
	}
	printf("\n");

	// Sprites & tiles
	uint8_t sprites_row[128][128];

	if (c_sprites) {
		printf("Sprites:\n");
		// Fill the sprites_row buffer
		for (int i = 0; i < c_sprites->chunk_size; i++) {
			// Extract colours from byte (each colour is a nibble)
			// A nibble is a 0-15 palette index value 
			uint8_t index1 = c_sprites->data[i] & 15;
			uint8_t index2 = c_sprites->data[i] >> 4;

			// Calculate their position in the buffer
			int little_column = i % 4;
			int little_row = i / 4 % 8;
			int big_column = i / 32 % 16;
			int big_row = i / 512;

			int x = little_column * 2 + big_column * 8;
			int y = little_row + big_row * 8;

			sprites_row[y][x] = index1;
			sprites_row[y][x + 1] = index2;
		}
		int pixels_rendered = 0;
		// Render the buffer (preview of the sprite map)
		for (int row = 0; row < 16; row++) {
			for (int half = 0; half < 2; half++) {
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 64; j++) {
						colour_t c = palette[sprites_row[i + row * 8][j + (half * 64)] & 15];
						
						printf("\e[48;2;%d;%d;%dm  ", c.r, c.g, c.b);
						pixels_rendered++;
					}
					printf("\e[0m\n");
				}
			}
			if (pixels_rendered > c_sprites->chunk_size * 2)
				// The rest of the buffer is empty (perhaps with memory garbage)
				// TIC format ommits all trailing zeroes (empty sprite slots)
				break;
		}
	}

	if (c_tiles) {
		printf("Tiles:\n");
		// Fill the sprites_row buffer
		for (int i = 0; i < c_tiles->chunk_size; i++) {
			// Extract colours from byte (each colour is a nibble)
			// A nibble is a 0-15 palette index value 
			uint8_t index1 = c_tiles->data[i] & 15;
			uint8_t index2 = c_tiles->data[i] >> 4;

			// Calculate their position in the buffer
			int little_column = i % 4;
			int little_row = i / 4 % 8;
			int big_column = i / 32 % 16;
			int big_row = i / 512;

			int x = little_column * 2 + big_column * 8;
			int y = little_row + big_row * 8;

			sprites_row[y][x] = index1;
			sprites_row[y][x + 1] = index2;
		}
		int pixels_rendered = 0;
		// Render the buffer (preview of the tile map)
		for (int row = 0; row < 16; row++) {
			for (int half = 0; half < 2; half++) {
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 64; j++) {
						colour_t c = palette[sprites_row[i + row * 8][j + (half * 64)] & 15];
						
						printf("\e[48;2;%d;%d;%dm  ", c.r, c.g, c.b);
						pixels_rendered++;
					}
					printf("\e[0m\n");
				}
			}
			if (pixels_rendered > c_tiles->chunk_size * 2)
				// The rest of the buffer is empty (perhaps with memory garbage)
				// TIC format ommits all trailing zeroes (empty tile slots)
				break;
		}
	}

	// Code
	char *code = (char*) malloc(c_code->chunk_size + 1);
	memcpy(code, c_code->data, c_code->chunk_size);
	
	for (int i = 0; i < c_code->chunk_size; i++) {
		if (code[i] == '\t')
			// Replace all tabs with spaces because for some reason my terminal cannot
			// properly print tabs so whatever
			code[i] = ' ';
	}
	// Revising this code a couple of months later, I am not sure whether this was
	// really necessary or I just wanted to be safe, but I chose to terminate the
	// string with a null character anyway.
	code[c_code->chunk_size] = '\0';
	
	printf("Code:\n\e[32m%s\n\e[0m", code);

	if (argc == 2) {
		printf("Done.\n");
	}

	// Code export
	else if (!strcmp(argv[2], "-get-code")) {
		// Output to a file
		FILE *code_file = fopen(argv[3], "wb");
		fwrite(c_code->data, 1, c_code->chunk_size, code_file);
		fclose(code_file);
		printf("Successfully exported the code.\n");
	}

	// Insert code
	else if (!strcmp(argv[2], "-set-code")) {
		// Load code file and calculate some of its header info
		FILE *code_file = fopen(argv[3], "rb");
		
		uint8_t c_code_info = c_code->bank_number << 5 | c_code->chunk_type;
		char buffer[0x10000];
		uint16_t code_size = 0;
		
		// Read the code and count how many characters are in it

		// I know there were simpler and more efficient ways of
		// figuring out the file size but ok
		while (1) {
			char c;
			if (fread(&c, 1, 1, code_file) < 1) break;
			buffer[code_size] = c;
			code_size++;
			// Checks for overflow of the size variable
			if (!code_size) {
				// Unable to insert it into the cartridge: unsurpassable limitation of the .tic format
				printf("Error! Code too large.\n");
				fclose(code_file);
				return 0;
			}
		}
		printf("Code size: %d (%04X)\n", code_size, code_size);
		
		FILE *cart = fopen(argv[1], "wb");
		
		// Reinsert all the previous data of the cartridge back (rewrite the cartridge)
		for (chunk_t *current = file_data; current; current = current->next) {
			// Skip the old code part (to insert the updated one at the end)
			if (current->chunk_type == 5) continue;
			// printf("current chunk's type: %d\n", current->chunk_type);
		
			uint8_t chunk_info = current->bank_number << 5 | current->chunk_type;	
			
			fwrite(&chunk_info, 1, 1, cart);
			fwrite(&current->chunk_size, 2, 1, cart);
			fseek(cart, 1, SEEK_CUR);
			fwrite(current->data, 1, current->chunk_size, cart);
		}
		
		// Insert the updated chunk of code
		fwrite(&c_code_info, 1, 1, cart);
		fwrite(&code_size, 2, 1, cart);
		fseek(cart, 1, SEEK_CUR);
		fwrite(buffer, 1, code_size, cart);

		fclose(code_file);
		fclose(cart);

		printf("Successfully inserted the code.\n");
	}

	// Cover image export
	else if (!strcmp(argv[2], "-get-cover")) {
		// Check for cover
		if (c_cover) {
			FILE *cover = fopen(argv[3], "wb");
			fwrite(c_cover->data, 1, c_cover->chunk_size, cover);
			fclose(cover);
			printf("Successfully exported the cover gif.\n");
		}
		else {
			printf("Cover not present.\n");
		}
	}

	// Cover image import

	// The great thing about this particular part of the program is: it doesn't care
	// about the gif contents, if it is the right size, whether it is animated or not...
	// This means that you might want to take extra care and make sure your gif isn't
	// corrupted or anything (although if it is, probably nothing bad will happen), but
	// it does open up the possibilities for very interesting covers, those of which would
	// be impossible to get naturally (without any external tool just like this one).
	
	// The key here is, TIC already accepts gifs with full colour, and if you export an
	// animated one, the full animation will be visible on the website, but only the last
	// frame will be displayed on SURF mode (at least it doesn't break). A cover with the
	// wrong dimensions, animated or not, will be broken in SURF mode, but will still
	// show up on the website though.

	// Use it wisely.
	else if (!strcmp(argv[2], "-set-cover")) {
		// Load cover file and calculate some of its header info
		FILE *cover_file = fopen(argv[3], "rb");
		
		uint8_t c_cover_info = 0x03; // Constant
		uint8_t buffer[0x10000];
		uint16_t cover_size = 0;
		
		// Read the cover file and count how many bytes are in it

		// I know there were simpler and more efficient ways of
		// figuring out the file size but ok
		while (1) {
			uint8_t b;
			if (fread(&b, 1, 1, cover_file) < 1) break;
			buffer[cover_size] = b;
			cover_size++;
			// Checks for overflow of the size variable
			if (!cover_size) {
				// Unable to insert it into the cartridge: unsurpassable limitation of the .tic format
				printf("Error! Cover file too large.\n");
				fclose(cover_file);
				return 0;
			}
		}
		printf("Cover size: %d (%04X)\n", cover_size, cover_size);
		
		FILE *cart = fopen(argv[1], "wb");
		
		// Reinsert all the previous data of the cartridge
		for (chunk_t *current = file_data; current; current = current->next) {
			// Skip the old cover part (to insert the updated one at the end)
			if (current->chunk_type == 3) continue;
			// printf("current chunk's type: %d\n", current->chunk_type);
		
			uint8_t chunk_info = current->bank_number << 5 | current->chunk_type;	
			
			fwrite(&chunk_info, 1, 1, cart);
			fwrite(&current->chunk_size, 2, 1, cart);
			fseek(cart, 1, SEEK_CUR);
			fwrite(current->data, 1, current->chunk_size, cart);
		}
		
		// Insert the updated chunk of cover
		fwrite(&c_cover_info, 1, 1, cart);
		fwrite(&cover_size, 2, 1, cart);
		fseek(cart, 1, SEEK_CUR);
		fwrite(buffer, 1, cover_size, cart);

		fclose(cover_file);
		fclose(cart);

		printf("Successfully inserted the cover gif.\n");
	}

	return 0;
}