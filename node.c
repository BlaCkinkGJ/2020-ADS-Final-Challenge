
#include "assert.h"
#include "card.h"
#include "index.h"
#include <malloc.h>
#include <stdio.h>

/**
 * @brief 브랜치를 초기화 합니다.
 *
 * @param b 초기화 시킬 브랜치에 해당합니다.
 */
static void RTreeInitBranch(struct Branch *b)
{
	RTreeInitRect(&(b->rect));
	b->child = NULL;
}

/**
 * @brief 노드를 초기화 하도록 합니다.
 *
 * @param N 초기화 시킬 노드에 해당합니다.
 */
void RTreeInitNode(struct Node *N)
{
	register struct Node *n = N;
	register int i;
	n->count = 0;
	n->level = -1;
	for (i = 0; i < MAXCARD; i++)
		RTreeInitBranch(&(n->branch[i]));
}

/**
 * @brief 모든 브랜치의 셀이 비어있는 새로운 노드를 만듭니다.
 *
 * @return 새롭게 생성된 노트 n을 반환하도록 합니다.
 */
struct Node *RTreeNewNode()
{
	register struct Node *n;

	// n = new Node;
	n = (struct Node *)malloc(sizeof(struct Node));
	assert(n);
	RTreeInitNode(n);
	return n;
}

void RTreeFreeNode(struct Node *p)
{
	assert(p);
	// delete p;
	free(p);
}

extern void RTreeTabIn(int depth)
{
	int i;
	for (i = 0; i < depth; i++)
		putchar('\t');
}

/**
 * @brief 노드의 브랜치들 안의 모든 사각형을 포함하는 가장 작은 사각형을 찾습니다.
 *
 * @param N 노드를 가리키는 포인터입니다.
 *
 * @return 가장 작은 사각형에 해당합니다.
 */
struct Rect RTreeNodeCover(struct Node *N)
{
	register struct Node *n = N;
	register int i, first_time = 1;
	struct Rect r;
	assert(n);

	RTreeInitRect(&r);
	for (i = 0; i < MAXKIDS(n); i++)
		if (n->branch[i].child) {
			if (first_time) {
				r = n->branch[i].rect;
				first_time = 0;
			} else
				r = RTreeCombineRect(&r, &(n->branch[i].rect));
		}
	return r;
}

/**
 * @brief 브랜치를 선택합니다.
 *
 * @details 최소한의 증가 면적을 가지는 새로운 사각형을 만드는 브랜치를 선택합니다.
 * 현재 노드 안에서의 사각형들을 포함하는 전체 면적을 최소화 하는 결과를 가져니다.
 *
 * 선택된 것이 같은 값을 가지는 경우에는 검색의 효율성을 위해서 면적이 더 적은 것을 선택하도록 합니다.
 *
 * @param R 사각형을 가리키는 포인터에 해당합니다.
 * @param N 노드를 가리키는 포인터에 해당합니다.
 *
 * @return 최적의 브랜치 번호에 해당합니다.
 */
int RTreePickBranch(struct Rect *R, struct Node *N)
{
	register struct Rect *r = R;
	register struct Node *n = N;
	register struct Rect *rr;
	register int i, first_time = 1;
	RectReal increase, bestIncr = (RectReal)-1, area, bestArea;
	int best;
	struct Rect tmp_rect;
	assert(r && n);

	for (i = 0; i < MAXKIDS(n); i++) {
		if (n->branch[i].child) {
			rr = &n->branch[i].rect;
			area = RTreeRectSphericalVolume(rr);
			tmp_rect = RTreeCombineRect(r, rr);
			increase = RTreeRectSphericalVolume(&tmp_rect) - area;
			if (increase < bestIncr || first_time) {
				best = i;
				bestArea = area;
				bestIncr = increase;
				first_time = 0;
			} else if (increase == bestIncr && area < bestArea) {
				best = i;
				bestArea = area;
				bestIncr = increase;
			}
		}
	}
	return best;
}

/**
 * @brief 노드에 branch를 추가하고, 필요하다면 노드를 분리를 하도록 합니다.
 *
 * @param B branch 정보를 가리키는 포인터입니다.
 * @param N node를 가리키는 포인터입니다.
 * @param New_node 새로운 노트의 포인터에 대한 주소를 갑니다.
 *
 * @return 노드가 split 되지 않은 경우에는 0이 반환되고, split된 경우에는 1이 반환됩니다.
 */
int RTreeAddBranch(struct Branch *B, struct Node *N, struct Node **New_node)
{
	register struct Branch *b = B;
	register struct Node *n = N;
	register struct Node **new_node = New_node;
	register int i;

	assert(b);
	assert(n);

	if (n->count < MAXKIDS(n)) /**< split이 필요하지 않을 것으로 보임 */
	{
		for (i = 0; i < MAXKIDS(n); i++) /**< 빈 브랜치를 탐색 */
		{
			if (n->branch[i].child == NULL) {
				n->branch[i] = *b;
				n->count++;
				break;
			}
		}
		return 0;
	} else {
		assert(new_node);
		RTreeSplitNode(n, b, new_node);
		return 1;
	}
}

/**
 * @brief 현재 종속하는 노드의 연결을 끊습니다.
 *
 * @param n 노드를 가리키는 포인터입니다.
 * @param i branch 번호에 해당합니다.
 */
void RTreeDisconnectBranch(struct Node *n, int i)
{
	assert(n && i >= 0 && i < MAXKIDS(n));
	assert(n->branch[i].child);

	RTreeInitBranch(&(n->branch[i]));
	n->count--;
}
