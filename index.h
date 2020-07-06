#ifndef _INDEX_
#define _INDEX_

#include <stdbool.h>

#define PGSIZE 4096
#define NUMDIMS 2 /* 차원의 수 */
#define NDEBUG

#ifndef tid_t
#include <stdio.h>

typedef size_t tid_t;
#endif

typedef double RectReal;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define NUMSIDES 2 * NUMDIMS

struct Rect {
	bool is_use;
	RectReal boundary[NUMSIDES]; /* xmin,ymin,...,xmax,ymax,... */
};

struct Node;

struct Branch {
	struct Rect rect;
	struct Node *child;
};

/**
 * @brief 노드가 할 수 있는 최대 브랜칭의 수입니다.
 */
#define MAXCARD (int)((PGSIZE - (2 * sizeof(int))) / sizeof(struct Branch))

struct Node {
	int count;
	int level; /* 0 is leaf, others positive */
	struct Branch branch[MAXCARD];
};

struct ListNode {
	struct ListNode *next;
	struct Node *node;
};

/**
 * @brief 탐색 시에 호출되는 callback 함수의 원형에 해당한다.
 */
typedef int (*SearchHitCallback)(int id, void *arg);

extern int RTreeSearch(struct Node *, struct Rect *, SearchHitCallback, void *);
extern int RTreeInsertRect(struct Rect *, tid_t, struct Node **, int depth);
extern int RTreeDeleteRect(struct Rect *, tid_t, struct Node **);
extern struct Node *RTreeNewIndex();
extern struct Node *RTreeNewNode();
extern void RTreeInitNode(struct Node *);
extern void RTreeFreeNode(struct Node *);
extern void RTreeTabIn(int);
extern struct Rect RTreeNodeCover(struct Node *);
extern void RTreeInitRect(struct Rect *);
extern RectReal RTreeRectArea(struct Rect *);
extern RectReal RTreeRectSphericalVolume(struct Rect *R);
extern struct Rect RTreeCombineRect(struct Rect *, struct Rect *);
extern int RTreeOverlap(struct Rect *, struct Rect *);
extern int RTreeAddBranch(struct Branch *, struct Node *, struct Node **);
extern int RTreePickBranch(struct Rect *, struct Node *);
extern void RTreeDisconnectBranch(struct Node *, int);
extern void RTreeSplitNode(struct Node *, struct Branch *, struct Node **);

extern int RTreeSetNodeMax(int);
extern int RTreeSetLeafMax(int);
extern int RTreeGetNodeMax();
extern int RTreeGetLeafMax();

#endif /* _INDEX_ */
