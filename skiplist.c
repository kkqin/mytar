#include "mytar.h"

struct _node* create_node(int level, int key, object* obj) {
        struct _node* nd = (struct _node*)malloc(sizeof(struct _node));
        nd->key = key;
        nd->obj = obj;

        for(int i = 0; i < level; i++)
                nd->forward[i] = NULL;

        return nd;
}

struct _skiplist* create_skiplist(void) {
        struct _skiplist* skp = (struct _skiplist*)malloc(sizeof(struct _skiplist));
        skp->level = 1;
        skp->head = create_node(MAX_LEVEL, 0, 0);

        return skp;
}

static int random_level()
{
        int level = 1;
        while ((rand() & 0xFFFF) < (0.5 * 0xFFFF)) {
                level += 1;
        }
        return (level < MAX_LEVEL) ? level : MAX_LEVEL;
}

void insert_node(struct _skiplist* skp, int key, object* obj) {
        struct _node *update[MAX_LEVEL];
        struct _node* p, *q;
        p = skp->head;
        int level = skp->level;

        for(; level >= 0; level--) {
                while(p->forward[level] != NULL && p->forward[level]->key < key)
                        p = p->forward[level]; // 这级中往前走
                update[level] = p; // 找到最后的位置
        }

        int lv = random_level();
        if(lv > skp->level) { // 最大级更新
                for(int i = skp->level; i < lv; i++)
                        update[i] = skp->head;
                skp->level = lv;
        }

        struct _node* nd = create_node(lv, key, obj);
        for(int h_lv = skp->level; h_lv >= 0; h_lv--) {
                nd->forward[h_lv] = update[h_lv]->forward[h_lv]; // 新节点的尾等于位置的尾
                update[h_lv]->forward[h_lv] = nd;
        }
}

struct _node *find(struct _skiplist *skp, int key)
{
        struct _node *nd;
        int i;

        nd = skp->head;
        for (i = skp->level - 1; i >= 0; i--) {
                while (nd->forward[i] != NULL) {
                        if (nd->forward[i]->key < key)
                                nd = nd->forward[i];
                        else if (nd->forward[i]->key == key)
                                return nd->forward[i];
                        else
                                break;
                }
        }

        return NULL;
}


