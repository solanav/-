#ifndef LWH_H
#define LWH_H

typedef struct _Hex_data Hex_data;

int print_hex(Hex_data *hex_data, int column_size);
Hex_data *read_hex_p(void *pointer, int size);
Hex_data *read_hex_f(FILE *pointer);
void delete_hex_data(Hex_data *hex_data);
void *find_hex(Hex_data *hex_data, int starting_offset,
	       unsigned char to_find);
int find_n_hex(Hex_data *hex_data, unsigned char *to_find,
	       int size_to_find);
int write_byte(void *pointer, unsigned char new_value);
int save_bin(char *file_name, Hex_data *hex_data);

#endif