#include <sys/msg.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "kq.h"

#define KQ_MAX 1000

struct msgbuf{
    long mtype;
    char mdata[KQ_MAX+1];
};

void init_kq(struct kq* k, key_t outgoing, key_t incoming){
    k->outgoing = outgoing;
    k->incoming = incoming;

    msgget(k->outgoing, 0777 | IPC_CREAT);
    msgget(k->incoming, 0777 | IPC_CREAT);
}

_Bool insert_kq(struct kq* k, char* msg, uint8_t mtype){
    int msgid = msgget(k->outgoing, 0777);
    struct msgbuf buf = {0};
    buf.mtype = mtype;
    strncpy(buf.mdata, msg, KQ_MAX);

    return !msgsnd(msgid, &buf, strnlen(buf.mdata, KQ_MAX), 0);
}

uint8_t* pop_kq(struct kq* k, uint8_t* mtype){
    int msgid = msgget(k->incoming, 0777), br;
    struct msgbuf buf = {0};
    uint8_t* ret;

    br = msgrcv(msgid, &buf, KQ_MAX, 0, 0);
    ret = malloc(br+1);
    memcpy(ret, buf.mdata, br);
    ret[br] = 0;
    if(mtype)*mtype = buf.mtype;

    return ret;
}
