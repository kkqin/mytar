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
	//srand(time(NULL));
        int level = 1;
        while ((rand() & 0xFFFF) < (0.5 * 0xFFFF)) {
                level += 1;
        }
        return (level < MAX_LEVEL) ? level : MAX_LEVEL;
	/*int level = 1;
        const double p = 0.25;
        while ((rand() & 0xffff) < 0xffff * p) {
                level++;
        }
        return level > MAX_LEVEL ? MAX_LEVEL : level;*/
}

void insert_node(struct _skiplist* skp, int key, object* obj) {
        struct _node *update[MAX_LEVEL];
        struct _node* p;
        p = skp->head;
        int level = skp->level;

        for(int i = level - 1; i >= 0; i--) {
                while(p->forward[i] != NULL && p->forward[i]->key < key)
                        p = p->forward[i]; // 这级中往前走
                update[i] = p; // 找到最后的位置
        }

        int lv = random_level();
        if(lv > skp->level) { // 最大级更新
                for(int i = skp->level; i < lv; i++)
                        update[i] = skp->head;
                skp->level = lv;
        }

        p = create_node(lv, key, obj);
        for(int i = 0; i < lv; i++) {
                p->forward[i] = update[i]->forward[i]; // 新节点的尾等于位置的尾
                update[i]->forward[i] = p;
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

void print(skiplist *sl)
{
        node *nd;
        int i;

        for (i = 0; i <= MAX_LEVEL; i++) {
                nd = sl->head->forward[i];
                printf("Level[%d]:", i);

                while (nd) {
                        printf("%d -> ", nd->key);
                        nd = nd->forward[i];
                }
                printf("\n");
        }
}


