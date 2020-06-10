
#ifndef __QUEUE_H__
#define __QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define QUEUE_MAX_MSG_SIZE      (127U)

void queue_init();
void enqueue(char *item);
char* dequeue();
int isEmpty();

void print_queue();


#ifdef __cplusplus
}
#endif

#endif /**< __MESH_UTILS_H__ */
