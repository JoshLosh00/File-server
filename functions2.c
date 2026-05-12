#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vector2.h"
#include <poll.h>

void pvector_init(struct pvector *v, size_t cap){

        v->size = 0;
        v->capacity = cap;

        v->data = malloc(v->capacity * sizeof(struct fdinfo));
}

void pvector_push(struct pvector *v, const struct fdinfo c){
        
        if (v->size+1 > v->capacity){
                v->capacity *= 2;
                struct fdinfo *tmp = realloc(v->data, v->capacity * sizeof(struct fdinfo));
                if (tmp == NULL){
                        printf("malloc error\n");
                        return;  
                }
                v->data = tmp;
        }

        v->data[v->size] = c;
        v->size++;
}

/*
void push_bytes(struct vector *v, const char *s, size_t length){

        size_t old = v->size;
        v->size += length;

        if (v->size > v->capacity){
                v->capacity = v->size*2;
                char *tmp = realloc(v->data, v->capacity);
                if (!tmp){
                        printf("malloc error\n");
                        return;  
                }
                v->data = tmp;
        }

        memcpy(v->data + old, s, length);
}
*/

/*
void vector_string_termination(struct vector *v){
        if (v->size == v->capacity){
                v->capacity++;
                realloc(v->data, v->capacity);
        }
        v->data[v->size] = '\0'; 
}
*/


void pvector_free(struct pvector *v){

        free(v->data);

}
