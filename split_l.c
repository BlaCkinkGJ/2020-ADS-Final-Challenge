
#include "split_l.h"
#include "assert.h"
#include "card.h"
#include "index.h"
#include <stdio.h>

/**
 * @brief 꽉찬 노드로 부터 브랜치들을 브랜치 버퍼로 가져와서 추가적인 브랜치에 추가하도록 합니다.
 *
 * @param N 꽉찬 노드를 가리킵니다.
 * @param B 추가적인 브랜치에 해당합니다.
 */
static void RTreeGetBranches(struct Node *N, struct Branch *B)
{
	register struct Node *n = N;
	register struct Branch *b = B;
	register int i;

	assert(n);
	assert(b);

	for (i = 0; i < MAXKIDS(n); i++) { /**< 브랜치 버퍼로 가져옵니다. */
		assert(n->branch[i].child); /**< 엔트리가 꽉 찼는지 확인 */
		BranchBuf[i] = n->branch[i];
	}
	BranchBuf[MAXKIDS(n)] = *b; /**< 추가적인 브랜치를 넣어줍니다. */
	BranchCount = MAXKIDS(n) + 1;

	/* 집합에 있는 모든 Recctangle에 대해 계산합니다.*/
	CoverSplit = BranchBuf[0].rect;
	for (i = 1; i < MAXKIDS(n) + 1; i++) {
		CoverSplit = RTreeCombineRect(&CoverSplit, &BranchBuf[i].rect);
	}

	RTreeInitNode(n);
}

/**
 * @brief PartitionVars 구조체를 초기화 합니다.
 *
 * @param P 초기화 하고자 하는 PartitionVars 구조체 입니다.
 * @param maxrects 최대 사각형 갯수에 해당합니다.
 * @param minfill 차야하는 최소 값에 해당합니다.
 */
static void RTreeInitPVars(struct PartitionVars *P, int maxrects, int minfill)
{
	register struct PartitionVars *p = P;
	register int i;
	assert(p);

	p->count[0] = p->count[1] = 0;
	p->total = maxrects;
	p->minfill = minfill;
	for (i = 0; i < maxrects; i++) {
		p->taken[i] = FALSE;
		p->partition[i] = -1;
	}
}

/**
 * @brief 그룹 중 하나에 브랜치를 넣어주도록 합니다.
 *
 * @param i seed의 번호에 해당합니다.
 * @param group 그룹 번호에 해당합니다.
 * @param p 현재의 PartitionVars 포인터에 해당합니다.
 */
static void RTreeClassify(int i, int group, struct PartitionVars *p)
{
	assert(p);
	assert(!p->taken[i]);

	p->partition[i] = group;
	p->taken[i] = TRUE;

	if (p->count[group] == 0)
		p->cover[group] = BranchBuf[i].rect;
	else
		p->cover[group] =
			RTreeCombineRect(&BranchBuf[i].rect, &p->cover[group]);
	p->area[group] = RTreeRectSphericalVolume(&p->cover[group]);
	p->count[group]++;
}

/**
 * @details 2개 그룹의 첫번째 원소 집합에서 2개의 사각형을 선택합니다.
 * 선택된 2개는 현재 차원에서 가장 분리되어 있고, 겹치는 것을 최소화 되게 선택합니다.
 *
 * 분리와 겹침의 거리는 현재 차원에서의 모든 집합 공간에서의 폭(width)에 modulo한 값으로
 * 설정됩니다.
 *
 * @param P 파티션 변수 포인터
 */
static void RTreePickSeeds(struct PartitionVars *P)
{
	register struct PartitionVars *p = P;
	register int i, dim, high;
	register struct Rect *r, *rlow, *rhigh;
	register float w, separation, bestSep;
	RectReal width[NUMDIMS];
	int leastUpper[NUMDIMS], greatestLower[NUMDIMS];
	int seed0, seed1;
	assert(p);

	for (dim = 0; dim < NUMDIMS; dim++) {
		high = dim + NUMDIMS;

		/**
		 * @brief 현재 차원에서 각 방향에서 가장 먼 사각형들을  찾습니다.
		 */
		greatestLower[dim] = leastUpper[dim] = 0;
		for (i = 1; i < NODECARD + 1; i++) {
			r = &BranchBuf[i].rect;
			if (r->boundary[dim] >
			    BranchBuf[greatestLower[dim]].rect.boundary[dim]) {
				greatestLower[dim] = i;
			}
			if (r->boundary[high] <
			    BranchBuf[leastUpper[dim]].rect.boundary[high]) {
				leastUpper[dim] = i;
			}
		}

		/**
		 * @brief 현재 차원에서의 모든 집합의 폭을 구합니다.
		 */
		width[dim] =
			CoverSplit.boundary[high] - CoverSplit.boundary[dim];
	}

	/**
	 * @brief 최적의 분리 차원과 2개의 seed 사각형을 구합니다.
	 *
	 */
	for (dim = 0; dim < NUMDIMS; dim++) {
		high = dim + NUMDIMS;

		assert(width[dim] >= 0);
		if (width[dim] == 0)
			w = (RectReal)1;
		else
			w = width[dim];

		rlow = &BranchBuf[leastUpper[dim]].rect;
		rhigh = &BranchBuf[greatestLower[dim]].rect;
		if (dim == 0) {
			seed0 = leastUpper[0];
			seed1 = greatestLower[0];
			separation = bestSep =
				(rhigh->boundary[0] - rlow->boundary[NUMDIMS]) /
				w;
		} else {
			separation = (rhigh->boundary[dim] -
				      rlow->boundary[dim + NUMDIMS]) /
				     w;
			if (separation > bestSep) {
				seed0 = leastUpper[dim];
				seed1 = greatestLower[dim];
				bestSep = separation;
			}
		}
	}

	if (seed0 != seed1) {
		RTreeClassify(seed0, 0, p);
		RTreeClassify(seed1, 1, p);
	}
}

/**
 * @brief 그룹안에 있지 않은 사각형을 그룹에 넣어주도록 합니다.
 * @details pigeonhole이란? 비둘기집 원리로
 * "비둘기의 수보다 비둘기 집의 개수가 적을 경우,
 * 모든 비둘기 집에 한 마리 이상의 비둘기 들어갔다고하면 어느 집에는
 * 2 마리 이상의 비둘기가 들어갔다"를 의미합니다.
 *
 * @param P 현재 PartitionVars 구조체에 해당합니다.
 */
static void RTreePigeonhole(struct PartitionVars *P)
{
	register struct PartitionVars *p = P;
	struct Rect newCover[2];
	register int i, group;
	RectReal newArea[2], increase[2];

	for (i = 0; i < NODECARD + 1; i++) {
		if (!p->taken[i]) {
			/**
			 * @brief 하나의 그룹이 가득 찬 경우 다른 그룹에 사각형을 넣습니다.
			 */
			if (p->count[0] >= p->total - p->minfill) {
				RTreeClassify(i, 1, p);
				continue;
			} else if (p->count[1] >= p->total - p->minfill) {
				RTreeClassify(i, 0, p);
				continue;
			}

			/**
			 * @brief 이전 것과 새로운 것을 포함하는 2개의 그룹들의 면적을 찾습니다.
			 *
			 */
			for (group = 0; group < 2; group++) {
				if (p->count[group] > 0)
					newCover[group] = RTreeCombineRect(
						&BranchBuf[i].rect,
						&p->cover[group]);
				else
					newCover[group] = BranchBuf[i].rect;
				newArea[group] = RTreeRectSphericalVolume(
					&newCover[group]);
				increase[group] =
					newArea[group] - p->area[group];
			}

			/**
			 * @brief 그룹 안의 사각형 중 확장량이 최소한인 것을 넣습니다.
			 */
			if (increase[0] < increase[1])
				RTreeClassify(i, 0, p);
			else if (increase[1] < increase[0])
				RTreeClassify(i, 1, p);

			/**
			 * @brief 그룹 안의 사각형 중 최대한 적게 포함하는 것을 넣습니다.
			 */
			else if (p->area[0] < p->area[1])
				RTreeClassify(i, 0, p);
			else if (p->area[1] < p->area[0])
				RTreeClassify(i, 1, p);

			/**
			 * @brief 그룹 안의 사각형 중에 가장 적은 원소를 가진 그룹을 넣습니다.
			 */
			else if (p->count[0] < p->count[1])
				RTreeClassify(i, 0, p);
			else
				RTreeClassify(i, 1, p);
		}
	}
	assert(p->count[0] + p->count[1] == NODECARD + 1);
}

/**
 * @brief 파티션을 찾는 0번째 방법입니다.
 *
 * @param p 찾은 파티션 값이 들어갑니다.
 * @param minfill 최소 차야하는 값입니다.
 */
static void RTreeMethodZero(struct PartitionVars *p, int minfill)
{
	RTreeInitPVars(p, BranchCount, minfill);
	RTreePickSeeds(p);
	RTreePigeonhole(p);
}

/**
 * @brief 파티션에 기반해서 2개의 노드들에 버퍼로부터 브랜치에 복사해서 넣습니다.
 *
 * @param N 노드 1
 * @param Q 노드 2
 * @param P 파티션에 해당합니다.
 */
static void RTreeLoadNodes(struct Node *N, struct Node *Q,
			   struct PartitionVars *P)
{
	register struct Node *n = N, *q = Q;
	register struct PartitionVars *p = P;
	register int i;
	assert(n);
	assert(q);
	assert(p);

	for (i = 0; i < NODECARD + 1; i++) {
		if (p->partition[i] == 0)
			RTreeAddBranch(&BranchBuf[i], n, NULL);
		else if (p->partition[i] == 1)
			RTreeAddBranch(&BranchBuf[i], q, NULL);
		else
			assert(FALSE);
	}
}

/**
 * @brief 노드를 split 하도록 합니다. 노드의 브랜치들과 기타 등등을 2개의 노드로 변경합니다.
 * 새로운 노드들은 이전의 노드와 새로운 노드로 구성됩니다.
 *
 * @param n split의 대상이 되는 노드에 해당합니다.
 * @param b 현재의 branch 정보입니다.
 * @param nn node들에 대한 인덱스 정보를 가집니다.
 */
void RTreeSplitNode(struct Node *n, struct Branch *b, struct Node **nn)
{
	register struct PartitionVars *p;
	register int level;

	assert(n);
	assert(b);

	/**
	 * @brief 모든 브랜치를 버퍼에 넣고 이전의 노드를 초기화 합니다.
	 */
	level = n->level;
	RTreeGetBranches(n, b);

	/**
	 * 파티션을 찾도록 합니다. 이때, 0번 방법(Linear Split)을 사용해서 찾습니다.
	 */
	p = &Partitions[0];
	RTreeMethodZero(p, level > 0 ? MinNodeFill : MinLeafFill);

	/**
	 * @brief 현재 선택된 파티션에 따라서 2개의 노드를 버퍼에서 브랜치로 넣습니다.
	 *
	 */
	*nn = RTreeNewNode();
	(*nn)->level = n->level = level;
	RTreeLoadNodes(n, *nn, p);
	assert(n->count + (*nn)->count == NODECARD + 1);
}
