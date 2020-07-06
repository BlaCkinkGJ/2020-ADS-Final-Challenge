
#include "assert.h"
#include "index.h"
#include <stdio.h>
#include <stdlib.h>

#include <float.h>
#include <math.h>

#define BIG_NUM (FLT_MAX / 4.0)

#define Undefined(x) ((x)->boundary[0] > (x)->boundary[NUMDIMS])
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @brief 사각형을 원점으로 초기화하도록 합니다.
 *
 * @param R 초기화 시킬 사각형에 해당합니다.
 */
void RTreeInitRect(struct Rect *R)
{
	register struct Rect *r = R;
	register int i;
	for (i = 0; i < NUMSIDES; i++)
		r->boundary[i] = (RectReal)0;
}

const double UnitSphereVolumes[] = {
	0.000000, /* dimension   0 */
	2.000000, /* dimension   1 */
	3.141593, /* dimension   2 */
	4.188790, /* dimension   3 */
	4.934802, /* dimension   4 */
	5.263789, /* dimension   5 */
	5.167713, /* dimension   6 */
	4.724766, /* dimension   7 */
	4.058712, /* dimension   8 */
	3.298509, /* dimension   9 */
	2.550164, /* dimension  10 */
	1.884104, /* dimension  11 */
	1.335263, /* dimension  12 */
	0.910629, /* dimension  13 */
	0.599265, /* dimension  14 */
	0.381443, /* dimension  15 */
	0.235331, /* dimension  16 */
	0.140981, /* dimension  17 */
	0.082146, /* dimension  18 */
	0.046622, /* dimension  19 */
	0.025807, /* dimension  20 */
};
#if NUMDIMS > 20
#error "not enough precomputed sphere volumes"
#endif
#define UnitSphereVolume UnitSphereVolumes[NUMDIMS]

/**
 * @brief 현재 사각형이 담당하는 공간의 정확한 크기를 반환합니다.
 *
 * @param R 현재 사각형의 값
 *
 * @return 담당하는 공간의 크기
 */
RectReal RTreeRectSphericalVolume(struct Rect *R)
{
	register struct Rect *r = R;
	register int i;
	register double sum_of_squares = 0, radius;

	assert(r);
	if (Undefined(r))
		return (RectReal)0;
	for (i = 0; i < NUMDIMS; i++) {
		double half_extent =
			(r->boundary[i + NUMDIMS] - r->boundary[i]) / 2;
		sum_of_squares += half_extent * half_extent;
	}
	radius = sqrt(sum_of_squares);
	return (RectReal)(pow(radius, NUMDIMS) * UnitSphereVolume);
}

/**
 * @brief 두 개의 사각형을 병합을 진행합니다.
 *
 * @param R 병합 대상 1
 * @param Rr 병합 대상 2
 *
 * @return 병합된 사각형에 해당합니다.
 */
struct Rect RTreeCombineRect(struct Rect *R, struct Rect *Rr)
{
	register struct Rect *r = R, *rr = Rr;
	register int i, j;
	struct Rect new_rect;
	assert(r && rr);

	if (Undefined(r))
		return *rr;

	if (Undefined(rr))
		return *r;

	for (i = 0; i < NUMDIMS; i++) {
		new_rect.boundary[i] = MIN(r->boundary[i], rr->boundary[i]);
		j = i + NUMDIMS;
		new_rect.boundary[j] = MAX(r->boundary[j], rr->boundary[j]);
	}
	return new_rect;
}

/**
 * @brief 두 개의 사각형이 겹치는 지를 확인합니다.
 *
 * @param R 사각형 1
 * @param S 사각형 2
 *
 * @return 사각형의 겹침 여부
 */
int RTreeOverlap(struct Rect *R, struct Rect *S)
{
	register struct Rect *r = R, *s = S;
	register int i, j;
	assert(r && s);

	for (i = 0; i < NUMDIMS; i++) {
		j = i + NUMDIMS;
		if (r->boundary[i] > s->boundary[j] ||
		    s->boundary[i] > r->boundary[j]) {
			return FALSE;
		}
	}
	return TRUE;
}
