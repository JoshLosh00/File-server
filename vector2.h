#ifndef VECTOR_H
#define VECTOR_H
#include <poll.h>

/*
struct vector {
	char *data;
	size_t size;
	size_t capacity;
	};

void vector_init_new(struct vector *v, size_t cap);
//void vector_push_byte(struct vector *v, const char c);
void vector_push_bytes(struct vector *v, const char *s, size_t length);
void vector_free_new(struct vector *v);
void vector_string_termination(struct vector *v);
*/

struct fdinfo {
	struct pollfd poll;
	char *inbuffer;
	size_t read;
	size_t in_cap;
	char *outbuffer;
//	size_t out_cap;
	size_t sent;
	size_t out_length;
	char build_message;
//	char header_sent;
//	size_t header_length;
	FILE *file;
};

struct pvector {
	struct fdinfo *data;
	size_t size;
	size_t capacity;
	};

void pvector_init(struct pvector *v, size_t cap);
//void vector_push_byte(struct vector *v, const char c);
void pvector_push(struct pvector *v, const struct fdinfo s);
void pvector_free(struct pvector *v);
//void vector_string_termination(struct vector *v);

#endif
