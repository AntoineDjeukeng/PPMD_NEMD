#include "list.h"
#include <stdlib.h>
#include <string.h>

static void *xmalloc(size_t n){ void *p=malloc(n); if(!p){ abort(); } return p; }

static void frame_free_content(t_frame *fr){ if(!fr) return; free(fr->x); fr->x=NULL; }
static void frame_free_vf(t_frame *fr){ if(!fr) return; free(fr->v); fr->v=NULL; free(fr->f); fr->f=NULL; }
 
void traj_list_push_tail(t_traj *t, const t_frame *src)
{
    t_node *nd=(t_node*)xmalloc(sizeof(*nd));
    nd->fr=*src;
    nd->next=NULL;
    if(!t->tail) t->head=t->tail=nd;
    else { t->tail->next=nd; t->tail=nd; }
    t->size++;
    if(t->size>t->cap){
        t_node *old=t->head; t->head=old->next; if(!t->head) t->tail=NULL;
        frame_free_content(&old->fr); frame_free_vf(&old->fr); free(old); t->size--;

    }
}

void traj_list_clear(t_traj *t)
{
    t_node *p=t->head;
    while(p){ t_node *n=p->next; free(p->fr.x); free(p->fr.v); free(p->fr.f); free(p); p=n; }
    t->head=t->tail=NULL; t->size=0;
}
