#include <sys/msg.h>

struct kq{
    key_t outgoing, incoming;
};

void init_kq(struct kq* k, key_t outgoing, key_t incoming);
_Bool insert_kq(struct kq* k, char* msg, uint8_t mtype);
uint8_t* pop_kq(struct kq* k, uint8_t* mtype);
