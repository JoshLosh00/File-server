#ifndef VECTOR_H
#define VECTOR_H
#include <poll.h>
#pragma once

struct fdinfo {
	struct pollfd poll;
	char *inbuffer;
	size_t read;
	size_t in_cap;
	char *outbuffer;
	size_t sent;
	size_t out_length;
	char build_message;
	FILE *file;
};

struct pvector {
	struct fdinfo *data;
	size_t size;
	size_t capacity;
	};

void pvector_init(struct pvector *v, size_t cap);
void pvector_push(struct pvector *v, const struct fdinfo s);
void pvector_free(struct pvector *v);
void freefdinfo(struct fdinfo *info);
void freefdinfo2(struct fdinfo *info);

#endif
