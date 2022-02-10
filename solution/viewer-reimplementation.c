#include <allegro5/allegro.h>
#include <stdio.h>

typedef struct {
	FILE *fp;
	uint8_t buffer[16];
	uint8_t error;
} bits_reader_state_s;

void exit_library_error(const char *a1) {
	fprintf(stderr, "%s\n", a1);
	exit(1);
}

void exit_runtime_error(const char *a1) {
	fprintf(stderr, "runtime error: %s is false!", a1);
	exit(1);
}

// sub_2040
uint64_t read_n_bits_from_file_as_number(bits_reader_state_s *state, uint64_t wanted_bits) {
	uint64_t available_bits;
	uint64_t read_bits;
	uint64_t result;
	int read_byte;
	uint64_t bits_to_read;
	uint8_t read_byte_cpy;
	uint8_t *buffer;

	if (wanted_bits > 64) {
		exit_runtime_error("n <= 64");
	}
	if (wanted_bits == 0) {
		return 0;
	}

	buffer = state->buffer;
	available_bits = ((uint64_t *) buffer)[1];
	read_bits = 0;
	result = 0;

	while (1) {
		if (!available_bits) {
			buffer[0] = 0;
			if (!fread(buffer, 1, 1, state->fp)) {
				state->error = 1;
				return 0;
			}
			available_bits = 8;
		}
		read_byte = buffer[0];
		bits_to_read = min(wanted_bits - read_bits, available_bits);
		available_bits -= bits_to_read;
		read_bits += bits_to_read;
		((uint64_t *) buffer)[1] = available_bits;
		result = (read_byte >> available_bits) | (result << available_bits);
		buffer[0] = read_byte & ((1 << available_bits) - 1);
		if (wanted_bits <= read_bits) {
			break;
		}
	}

	return result;
}

// sub_1AE0
void adjust_pixel_to_color(
        void *bitmap_ptr,
        int column,
        int row,
        uint8_t r_diff,
        uint8_t g_diff,
        uint8_t b_diff) {
	uint8_t old_r, old_g, old_b;

	// To modyfikuje wartosci piksela, ale na poczatku decode_file_to_bitmap
	// ustawiamy wszystkie piksele na (0, 0, 0), dostajemy zwykle (0, 0, 0)
	// wiec de facto ustawia na r_diff, g_diff i b_diff

	struct ALLEGRO_COLOR allegro_color = al_get_pixel(bitmap_ptr, column, row);
	al_unmap_rgb(allegro_color, &old_r, &old_g, &old_b);
	allegro_color = al_map_rgb((uint8_t) (r_diff + old_r), (uint8_t) (g_diff + old_g), (uint8_t) (b_diff + old_b));
	al_put_pixel(column, row, allegro_color);
}

// sub_1B80
void decode_file_to_bitmap(
        void *bitmap_ptr,
        bits_reader_state_s *state,
        unsigned int width,
        unsigned int height) {
	struct ALLEGRO_COLOR allegro_color;
	uint64_t cmd;
	uint64_t bits_to_read;
	char r_diff;
	char g_diff;
	char b_diff;
	int64_t positions_advanced;
	unsigned int row_it;
	uint8_t b_it;
	int column_it;
	uint8_t r_it;
	uint8_t g_it;
	int advance_position_by;
	uint64_t result;
	char column_diff_power;
	char row_diff_power;
	float other_row;
	float other_column;
	uint8_t b;
	int current_column_number;
	uint64_t positions_to_advance;
	unsigned int current_row_number;
	uint8_t g;
	uint8_t r;

	allegro_color = al_map_rgb(0LL, 0LL, 0LL);
	al_clear_to_color(allegro_color);
	r = 0;
	g = 0;
	b = 0;
	current_row_number = 0;
	current_column_number = 0;
	// Opis:
	// cmd -- liczba 0-7, 3 bity, mówi jaką akcje wykonac nastepnie
	while (current_row_number < height) {
		cmd = read_n_bits_from_file_as_number(state, 3uLL);
		if (cmd == 7 || cmd == 1 || cmd == 6) {
			// 7/1/6 -> wczytaj po 1/3/8 bitow diffa od obecnych wartosc i zmien obecne wartosci rgb
			bits_to_read = (cmd == 7) ? 1 : (cmd == 1 ? 3 : 8);
			r += read_n_bits_from_file_as_number(state, bits_to_read);
			g += read_n_bits_from_file_as_number(state, bits_to_read);
			b += read_n_bits_from_file_as_number(state, bits_to_read);
		}
		if (cmd == 0 || cmd == 4) {
			// 0/4 -> advance position by N
			bits_to_read = cmd == 0 ? 4 : 10;
			advance_position_by = read_n_bits_from_file_as_number(state, bits_to_read);
			current_row_number += (current_column_number + advance_position_by) / width;
			current_column_number = (current_column_number + advance_position_by) % width;
		}
		if (cmd == 2) {
			// write 1 pixel & advance position by 1
			adjust_pixel_to_color(bitmap_ptr, current_column_number, current_row_number, r, g, b);
			current_column_number++;
			if (width == current_column_number) {
				++current_row_number;
				current_column_number = 0;
			}
		}
		if (cmd == 5) {
			// cos wczytuje, robi al_map_rgb(a,b,c) i al_draw_filled_rectangle | wypelnia prostokat
			column_diff_power = read_n_bits_from_file_as_number(state, 3uLL);
			row_diff_power = read_n_bits_from_file_as_number(state, 3uLL);
			allegro_color = al_map_rgb(r, g, b);
			other_column = (float) (current_column_number + (int64_t) (1 << (column_diff_power + 1)));
			other_row = (float) (current_row_number + (int64_t) (1 << (row_diff_power + 1)));
			al_draw_filled_rectangle(
			        (float) current_column_number,
			        (float) current_row_number,
			        other_column,
			        other_row,
			        allegro_color);
		}
		if (cmd == 3) {
			// Wypisuje zadaną liczbę pikseli, opcjonalnie zmienia wartosci r g b po kazdym pikselu w zakresi -2 do 1
			r_diff = read_n_bits_from_file_as_number(state, 2) - 2;
			g_diff = read_n_bits_from_file_as_number(state, 2) - 2;
			b_diff = read_n_bits_from_file_as_number(state, 2) - 2;
			positions_to_advance = read_n_bits_from_file_as_number(state, 8);

			row_it = current_row_number;
			column_it = current_column_number;
			r_it = r;
			g_it = g;
			b_it = b;
			positions_advanced = 0;
			while (positions_advanced < positions_to_advance) {
				adjust_pixel_to_color(bitmap_ptr, column_it, row_it, r_it, g_it, b_it);
				r_it += r_diff;
				g_it += g_diff;
				b_it += b_diff;
				column_it++;
				if (column_it == width) {
					++row_it;
					column_it = 0;
				}
				positions_advanced++;
			}
		}
	}
}

int main(int argc, char **argv) {
	FILE *input_file_ptr;
	unsigned int height;
	struct ALLEGRO_COLOR allegro_color;
	int64_t bitmap;
	int64_t bitmap_cpy;
	int64_t event_queue;
	int64_t display_event_source;
	int64_t keyboard_event_source;
	unsigned int width;
	bits_reader_state_s reader_state;
	int event_buffer[20];

	if (argc <= 1) {
		exit_library_error("Missing argument!");
	}
	input_file_ptr = fopen(argv[1], "rb");
	if (!input_file_ptr) {
		exit_runtime_error("(f = fopen(argv[1], \"rb\")) != nullptr");
	}
	reader_state.fp = input_file_ptr;
	((uint64_t **) reader_state.buffer)[2] = 0;
	reader_state.error = 0;
	width = read_n_bits_from_file_as_number(&reader_state, 24);
	height = read_n_bits_from_file_as_number(&reader_state, 24);

	if (reader_state.error || !width || !height) {
		exit_library_error("Reading header failed!");
	}
	if (!(uint8_t) al_install_system(84018945LL, atexit_handler)) {
		exit_runtime_error("al_init()");
	}
	if (!(uint8_t) al_init_primitives_addon()) {
		exit_runtime_error("al_init_primitives_addon()");
	}
	if (!(uint8_t) al_install_keyboard()) {
		exit_runtime_error("al_install_keyboard()");
	}

	al_display = al_create_display(width, height);
	if (!al_display) {
		exit_runtime_error("al_display = al_create_display(img_size_x, img_size_y)");
	}
	al_set_window_title(al_display, "Picture Viewer");
	allegro_color = al_map_rgb(0, 0, 0);
	al_clear_to_color(allegro_color);
	al_flip_display();
	bitmap = al_create_bitmap(width, height);
	bitmap_cpy = bitmap;
	if (!bitmap) {
		exit_runtime_error("bmp = al_create_bitmap(img_size_x, img_size_y)");
	}

	al_set_target_bitmap(bitmap);
	if (!al_lock_bitmap(bitmap_cpy, 0, 2)) {
		exit_runtime_error("al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY)");
	}
	decode_file_to_bitmap((void *) bitmap_cpy, &reader_state, width, height);
	al_unlock_bitmap(bitmap_cpy);
	al_set_target_backbuffer(al_display);
	event_queue = al_create_event_queue();
	if (!event_queue) {
		exit_runtime_error("(event_queue = al_create_event_queue())");
	}
	display_event_source = al_get_display_event_source(al_display);
	al_register_event_source(event_queue, display_event_source);
	keyboard_event_source = al_get_keyboard_event_source();
	al_register_event_source(event_queue, keyboard_event_source);
	al_draw_bitmap(bitmap_cpy, 0LL, 0.0);
	al_flip_display();
	do {
		al_wait_for_event(event_queue, event_buffer);
	} while (event_buffer[0] != 42 && (event_buffer[0] != 10 || event_buffer[8] != 59));

	fclose(reader_state.fp);

	return 0;
}
