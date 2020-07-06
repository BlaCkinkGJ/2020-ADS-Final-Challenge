
#include "index.h"
#include "assert.h"
#include "card.h"
#include <malloc.h>
#include <stdio.h>

/**
 * @brief 하나의 노드로 구성된 비어있는 새로운 인덱스를 만들도록 합니다.
 *
 * @return 새롭게 생성된 노드 x를 반환합니다.
 */
struct Node *RTreeNewIndex()
{
	struct Node *x;
	x = RTreeNewNode();
	x->level = 0; /* leaf */
	return x;
}

/**
 * @brief 매개 변수로 준 사각형에 겹쳐지는 모든 사각형을 반환합니다.
 *
 * @param N root에 해당합니다.
 * @param R 겹쳐지는 범위(탐색 범위)에 해당합니다.
 * @param shcb 데이터를 찾았을 때의 callback 함수에 해당합니다.
 * @param cbarg 추가적인 매개 변수 값입니다.
 *
 * @return 만난 사각형의 갯수를 반환합니다.
 */
int RTreeSearch(struct Node *N, struct Rect *R, SearchHitCallback shcb,
		void *cbarg)
{
	register struct Node *n = N;
	register struct Rect *r = R;
	register int hitCount = 0;
	register int i;

	assert(n);
	assert(n->level >= 0);
	assert(r);

	if (n->level > 0) { /**< 트리의 내장 노드의 경우 */
		for (i = 0; i < NODECARD; i++)
			if (n->branch[i].child &&
			    RTreeOverlap(r, &n->branch[i].rect)) {
				hitCount += RTreeSearch(n->branch[i].child, R,
							shcb, cbarg);
			}
	} else { /**< 트리의 leaf 노드의 경우 */
		for (i = 0; i < LEAFCARD; i++)
			if (n->branch[i].child &&
			    RTreeOverlap(r, &n->branch[i].rect)) {
				hitCount++;
				if (shcb) /**< callback 함수 부여 여부 확인 */
					if (!shcb((tid_t)n->branch[i].child,
						  cbarg))
						return hitCount; /**< callback 함수에서 에러가 발생한 경우 */
			}
	}
	return hitCount;
}

/**
 * @brief 인덱스 구조에 새로운 사각형 데이터를 넣어주도록 합니다.
 *
 * @details 트리를 재귀적으로 내려간 후에 다시 올라가면서 분할(split)을 합니다.
 * 0이 반환되는 경우에는 이전 노드가 갱신되었음을 의미하고,
 * 1이 반환되는 경우에는 분할이 발생을 하여 새로운 노드로 포인터를 설정해주도록 합니다.
 * 이전 노드는 2개 중 하나로 갱신되게 됩니다.
 *
 * @param r 사각형을 가리킵니다.
 * @param tid 사각형의 id에 해당합니다.
 * @param n 새로운 노드에 해당합니다.
 * @param new_node 새로운 노드를 가리키는 포인터의 포인터입니다.
 * @param level 삽입에서 leaf level 까지의 step 수를 의미합니다.
 *
 * @return 노드가 split이 된 경우 0을 안된 경우에는 1을 반환
 */
static int RTreeInsertRect2(struct Rect *r, tid_t tid, struct Node *n,
			    struct Node **new_node, int level)
{
	register int i;
	struct Branch b;
	struct Node *n2;

	assert(r && n && new_node);
	assert(level >= 0 && level <= n->level);

	/**
	 * @brief leaf 노드가 아닌 경우 아래로 계속 내려가도록 합니다.
	 *
	 */
	if (n->level > level) {
		i = RTreePickBranch(r, n);
		if (!RTreeInsertRect2(r, tid, n->branch[i].child, &n2, level)) {
			n->branch[i].rect =
				RTreeCombineRect(r, &(n->branch[i].rect));
			return 0;
		} else { /**< child가 분할된 경우에 해당합니다. */
			n->branch[i].rect = RTreeNodeCover(n->branch[i].child);
			b.child = n2;
			b.rect = RTreeNodeCover(n2);
			return RTreeAddBranch(&b, n, new_node);
		}
	} else if (n->level == level) {
		/**
		 * @brief: 삽입을 위한 level에 도달하면 추가하고자 하는 사각형을 추가하고 분할을 합니다.
		 */
		b.rect = *r;
		b.child = (struct Node *)tid;
		return RTreeAddBranch(&b, n, new_node);
	} else {
		assert(FALSE);
		return 0;
	}
}

/**
 * @brief index 구조에 사각형 데이터를 삽입합니다.
 *
 * @param R 삽입되는 사각형에 해당합니다.
 * @param Tid 삽입되는 사각형의 ID에 해당합니다.
 * @param Root R-Tree의 루트에 해당합니다.
 * @param Level leaf level에서 삽입까지 얼만큼 왔는 지를 확인하는 변수입니다.
 *
 * @return split이 발생한 경우 1을 반환, 그렇지 않은 경우 0을 반환합니다.
 */
int RTreeInsertRect(struct Rect *R, tid_t Tid, struct Node **Root, int Level)
{
	register struct Rect *r = R;
	register tid_t tid = Tid;
	register struct Node **root = Root;
	register int level = Level;
	register int i;
	register struct Node *newroot;
	struct Node *newnode;
	struct Branch b;
	int result;

	assert(r && root);
	assert(level >= 0 && level <= (*root)->level);
	for (i = 0; i < NUMDIMS; i++)
		assert(r->boundary[i] <= r->boundary[NUMDIMS + i]);

	if (RTreeInsertRect2(r, tid, *root, &newnode,
			     level)) { /**< 루트에 대해 split을 진행합니다.*/
		newroot = RTreeNewNode(); /**< 새로운 루트를 만들어 냅니다. */
		newroot->level = (*root)->level + 1;
		b.rect = RTreeNodeCover(*root);
		b.child = *root;
		RTreeAddBranch(&b, newroot, NULL);
		b.rect = RTreeNodeCover(newnode);
		b.child = newnode;
		RTreeAddBranch(&b, newroot, NULL);
		*root = newroot;
		result = 1;
	} else {
		result = 0;
	}

	return result;
}

// Allocate space for a node in the list used in DeletRect to
// store Nodes that are too empty.
//
static struct ListNode *RTreeNewListNode()
{
	return (struct ListNode *)malloc(sizeof(struct ListNode));
	// return new ListNode;
}

static void RTreeFreeListNode(struct ListNode *p)
{
	free(p);
}

// Add a node to the reinsertion list.  All its branches will later
// be reinserted into the index structure.
//
/**
 * @brief 노드를 재삽입 리스트에 넣어줍니다.
 * 향후 모든 브랜치들은 인덱스 구조체에 재삽입됩니다.
 *
 * @param n 재삽입 리스트에 들어갈 노드
 * @param ee 히스트 노드의 head
 */
static void RTreeReInsert(struct Node *n, struct ListNode **ee)
{
	register struct ListNode *l;

	l = RTreeNewListNode();
	l->node = n;
	l->next = *ee;
	*ee = l;
}

/**
 * @brief index 구조체의 일부분에서 루트가 아닌 사각형을 제거합니다.
 *
 * @details 재귀적으로 트리를 들어가서 루트로 올라가면서 merge를 진행합니다.
 *
 * @param R 지우고자 하는 사각형 정보
 * @param Tid 지우고자 하는 사각형의 id
 * @param N 노드를 가리키는 포인터
 * @param Ee node들의 리스트 head에 해당합니다.
 *
 * @return 레코드를 찾은 경우 0, 못 찾은 경우 1을 반환합니다.
 */
static int RTreeDeleteRect2(struct Rect *R, tid_t Tid, struct Node *N,
			    struct ListNode **Ee)
{
	register struct Rect *r = R;
	register tid_t tid = Tid;
	register struct Node *n = N;
	register struct ListNode **ee = Ee;
	register int i;

	assert(r && n && ee);
	assert(tid >= 0);
	assert(n->level >= 0);

	if (n->level > 0) { /**< 리프 노드가 아닌 경우*/
		for (i = 0; i < NODECARD; i++) {
			if (n->branch[i].child &&
			    RTreeOverlap(r, &(n->branch[i].rect))) {
				if (!RTreeDeleteRect2(r, tid,
						      n->branch[i].child, ee)) {
					if (n->branch[i].child->count >=
					    MinNodeFill)
						n->branch[i]
							.rect = RTreeNodeCover(
							n->branch[i].child);
					else {
						/**
						 * @brief 자식(child) 노드에 충분하지 않은 엔트리가 있는 경우에
						 * 자식 노드를 제거합니다.
						 *
						 */
						RTreeReInsert(
							n->branch[i].child, ee);
						RTreeDisconnectBranch(n, i);
					}
					return 0;
				}
			}
		}
		return 1;
	} else { /**< 리프 노드인 경우 */
		for (i = 0; i < LEAFCARD; i++) {
			if (n->branch[i].child &&
			    n->branch[i].child == (struct Node *)tid) {
				RTreeDisconnectBranch(n, i);
				return 0;
			}
		}
		return 1;
	}
}

/**
 * @brief index 구조로부터 사각형 데이터를 제거하도록 합니다.
 *
 * @param R 사각형을 가리키는 포인터
 * @param Tid 레코드의 id에 해당합니다.
 * @param Nn root node 포인터의 포인터에 해당합니다.
 *
 * @return 1은 레코드를 찾은 것이고, 0은 찾지 못한 것입니다.
 * @note root의 제거가 발생할 수 있습니다. 
 */
int RTreeDeleteRect(struct Rect *R, tid_t Tid, struct Node **Nn)
{
	register struct Rect *r = R;
	register tid_t tid = Tid;
	register struct Node **nn = Nn;
	register int i;
	register struct Node *tmp_nptr;
	struct ListNode *reInsertList = NULL;
	register struct ListNode *e;

	assert(r && nn);
	assert(*nn);
	assert(tid >= 0);

	if (!RTreeDeleteRect2(r, tid, *nn, &reInsertList)) {
		/**
		 * @brief 삭제할 데이터를 찾은 경우에 브랜치들로부터
		 * 제거되어진 노드들을 가져와서 재삽입을 진행합니다.
		 *
		 */
		while (reInsertList) {
			tmp_nptr = reInsertList->node;
			for (i = 0; i < MAXKIDS(tmp_nptr); i++) {
				if (tmp_nptr->branch[i].child) {
					RTreeInsertRect(
						&(tmp_nptr->branch[i].rect),
						(tid_t)tmp_nptr->branch[i].child,
						nn, tmp_nptr->level);
				}
			}
			/**
			 * @brief: 마지막으로 재삽입 리스트에 들어간 노드가
			 * leaf 노드이자 삭제 대상이므로 삭제해줍니다.
			 */
			e = reInsertList;
			reInsertList = reInsertList->next;
			RTreeFreeNode(e->node);
			RTreeFreeListNode(e);
		}

		/**
		 * @brief: leaf가 아니면서 1개의 child를 가지는
		 * 중복된 루트를 확인해서 제거합니다.
		 */
		if ((*nn)->count == 1 && (*nn)->level > 0) {
			for (i = 0; i < NODECARD; i++) {
				tmp_nptr = (*nn)->branch[i].child;
				if (tmp_nptr)
					break;
			}
			assert(tmp_nptr);
			RTreeFreeNode(*nn);
			*nn = tmp_nptr;
		}
		return 0;
	} else {
		return 1;
	}
}
