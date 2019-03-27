#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "lwh.h"

#define OK 0
#define ERROR -1

#define BUF 4096

struct _Hex_data {
	unsigned char *data;
	void *pointer;
	int size_of_data;
};

void test()
{
	int a = 0;

	if (a == 1)
		exit(0);

	printf("%d\n", a);
}

int print_hex(Hex_data *hex_data, int column_size)
{
	int i, size;
	unsigned char current_char = 0;
	void *current_pointer = NULL;

	if (!hex_data || column_size <= 0) {
		printf("[ERROR] Parameters to function are wrong\n");
		return ERROR;
	}

	for (i = 0; i < hex_data->size_of_data; i++) {
		current_char = (hex_data->data)[i];
		current_pointer = (hex_data->pointer) + i;

		if (i % column_size == 0)
			printf("%p > ", current_pointer);

		if (current_char <= 0xf)
			printf("0%x ", current_char);
		else
			printf("%x ", current_char);

		if ((i + 1) % column_size == 0)
			printf("\n");
	}

	return OK;
}

Hex_data *read_hex_p(void *pointer, int size)
{
	int i;
	Hex_data *hex_data = NULL;
	unsigned char *data = NULL;

	hex_data = (Hex_data *)calloc(1, sizeof(Hex_data));
	if (!hex_data) {
		printf("[ERROR] Memory error");
		return NULL;
	}

	data = (unsigned char *)calloc(size, sizeof(unsigned char));
	if (!data) {
		printf("[ERROR] Memory error");
		return NULL;
	}

	for (i = 0; i < size; i++) {
		data[i] = *((unsigned char *)pointer + i);
	}

	hex_data->data = data;
	hex_data->pointer = pointer;
	hex_data->size_of_data = size;

	return hex_data;
}

Hex_data *read_hex_f(FILE *pointer)
{
	int i = 0, size = BUF, c = 0;
	Hex_data *hex_data = NULL;
	unsigned char *data = NULL;

	hex_data = (Hex_data *)calloc(1, sizeof(Hex_data));
	if (!hex_data) {
		printf("[ERROR] Memory error\n");
		return NULL;
	}

	data = (unsigned char *)calloc(size, sizeof(unsigned char));
	if (!data) {
		printf("[ERROR] Memory error\n");
		return NULL;
	}

	while ((c = getc(pointer)) != EOF) {
		data[i] = c;
		i++;
		if (i == (size - 1)) {
			printf("[INFO] Reallocating from %d to %d\n", size,
			       size + BUF);
			size = size + BUF;
			data = (unsigned char *)realloc(
				data, size * sizeof(unsigned char));
			if (!data) {
				printf("[ERROR] Realloc failed\n");
				return NULL;
			}
			memset(data + (size - BUF), 0, BUF);
		}
	}

	hex_data->data = data;
	hex_data->pointer = pointer;
	hex_data->size_of_data = i;

	return hex_data;
}

void delete_hex_data(Hex_data *hex_data)
{
	if (!hex_data)
		return;

	free(hex_data->data);
	free(hex_data);

	return;
}

void *find_hex(Hex_data *hex_data, int starting_offset,
	       unsigned char to_find)
{
	int found = ERROR, offset = starting_offset;
	unsigned char current = 0;

	while (offset <= hex_data->size_of_data && found == ERROR) {
		current = (hex_data->data)[offset];

		offset++;

		if (current == to_find)
			found = OK;
	}

	if (found == ERROR)
		return NULL;

	return (hex_data->pointer) + (offset - 1);
}

int find_n_hex(Hex_data *hex_data, unsigned char *to_find,
	       int size_to_find)
{
	int i, found = ERROR, offset = ERROR;
	void *tmp = NULL;

	while (offset <= hex_data->size_of_data && found == ERROR) {
		tmp = find_hex(hex_data, offset + 1, to_find[0]);
		offset = tmp - hex_data->pointer;

		if (!tmp)
			return ERROR;

		found = OK;
		for (i = 0; i < size_to_find; i++) {
			if (to_find[i] != (hex_data->data)[offset + i]) {
				found = ERROR;
				break;
			}
		}
	}

	if (found == ERROR)
		return ERROR;

	return offset;
}

int rm_page_protect(void *addr)
{
	int page_size = getpagesize();

	if (!addr)
		return ERROR;

	addr -= (unsigned long)addr % page_size;

	if (mprotect(addr, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) == -1)
		return ERROR;

	return OK;
}

int write_byte(void *pointer, unsigned char new_value)
{
	*((unsigned char *)pointer) = new_value;

	return OK;
}

int save_bin(char *file_name, Hex_data *hex_data)
{
	int i = 0;
	FILE *bin_file = NULL;

	bin_file = fopen(file_name, "wb+");
	if (!bin_file)
		return ERROR;

	for (i = 0; i < hex_data->size_of_data; i++) {
		fputc((hex_data->data)[i], bin_file);
	}

	fclose(bin_file);

	return OK;
}

int main(int argc, char **argv)
{
	int tmp = 0, c = 0, i = 0;
	FILE *bin_file = NULL;
	Hex_data *hex_data_otf = NULL;
	Hex_data *hex_data_f = NULL;
	unsigned char to_find[] = { 0x00, 0x00, 0x00, 0x00, 0x83,
				    0x7d, 0xfc, 0x01 };

	test();

	// OTF Clean dump
	hex_data_otf = read_hex_p(&test, 32);
	print_hex(hex_data_otf, 8);

	tmp = find_n_hex(hex_data_otf, to_find, 8);
	if (tmp == ERROR) {
		printf("[ERROR] Can't find the bin data in otf dump\n");
		return OK;
	}

	// Write to clean dump
	rm_page_protect(&test);
	write_byte(&test + tmp, 0x01);

	// FILE Clean dump
	bin_file = fopen(argv[0], "rb");
	hex_data_f = read_hex_f(bin_file);

	tmp = find_n_hex(hex_data_f, to_find, 8);
	if (tmp == ERROR) {
		printf("[ERROR] Can't find the bin data in otf dump\n");
		return OK;
	}

	// Mod
	(hex_data_f->data)[tmp] = 0x01;

	// Write to file
	if ((i = strncmp(argv[0], "./a.out", 7)) == 0) {
		printf("[INFO] Trying to create a mod version\n");

		save_bin("mod", hex_data_f);
		
		printf("[DONE] Mod created\n");
	}

	// Check if OTF working
	test();

	// Clean
	fclose(bin_file);
	delete_hex_data(hex_data_otf);
	delete_hex_data(hex_data_f);

	return OK;
}

