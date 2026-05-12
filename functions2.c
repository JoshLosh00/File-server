#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vector.h"
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

void freefdinfo(struct fdinfo *info){
    close(info->poll.fd);
    info->poll.fd = -1;
    fclose(info->file);
    free(info->inbuffer);
    free(info->outbuffer);
}

void freefdinfo2(struct fdinfo *info){
    close(info->poll.fd);
    info->poll.fd = -1;
    //fclose(info->file);
    free(info->inbuffer);
    free(info->outbuffer);
}


void pvector_free(struct pvector *v){

        free(v->data);

}
