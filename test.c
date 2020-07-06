/**
 * @file test.c
 * @brief R-Tree를 기반으로 Final Challenge에서 요구하는 동작을 수행하도록 합니다.
 * @author Gijun Oh
 * @version 0.1
 * @date 2020-07-06
 */

#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_TABLE_SIZE ((0x1 << 20) + 1)
#define EPSILON (0.00001)

/**
 * @brief 탐색과 관련된 전역 변수에 해당합니다.
 */
static struct Rect *rect_tbl; /**< (id, rectangle)에 대한 정보를 가지는 테이블*/
static RectReal cx, cy;
static RectReal max_d_square;
static long max_id, nhits;
static RectReal cur_d;

/**
 * @brief Final Challenge에서 명시된 Command에 대한 열거형을 만듭니다.
 */
enum { INSERT = '+',
       ERASE = '-',
       SEARCH = '?',
};

/**
 * @brief 검색 중에 실행되는 Callback Function 입니다.
 *
 * @details Search 중에 이 함수가 불리게 되면 가장 먼저 점의 좌표를 구합니다.
 * `struct Rect`의 경우에 `xmin, ymin, xmax, ymax`로 구성되나,
 * 점의 경우 `xmin == xmax`이고 `ymin == ymax`이므로 `xmin`하고 `ymin`만 구하도록 합니다.
 *
 * xmin은struct Rect에서 boundary[0]에 해당하고, ymin은 boundary[1]에 해당합니다.
 * 그리고 이 x, y와 현재 원의 중심인 cx, cy와의 거리(d;d^2은 d_square)를 구합니다.
 *
 * 만약 원의 반지름(cur_d) 보다 d가 적은 경우에는 d가
 * 현재 기록된 최대 거리(max_d; max_d^2은 max_d_square)보다 크면 최대 id와 거리를 갱신하도록 합니다.
 *
 * 추가로 최대 거리가 d와 같은 경우에는 id가 좀 더 작은 녀석이 출력이 될 수 있도록 조정합니다.
 * 그리고 원 안에 있는 점이므로 점의 수를 1 증가 시켜줍니다.
 *
 * @param id 현재 원의 반지름을 기반으로 하는 사각형과 겹치는 점의 id
 * @param arg 사용되지 않음
 *
 * @return 문제가 없는 경우 1을 반환 (현재는 무조건 1을 반환하도록 되어 있습니다.)
 * @note double의 경우 같음의 비교에는 오차가 발생할 수 있으므로
 * fabs(double_value) < EPSILON으로 같다를 표기하도록 합니다.
 *
 * (EPSION은 아주 작은 수를 의미합니다.)
 */
int SearchCallback(int id, void *arg)
{
	struct Rect *r = &rect_tbl[id];
	RectReal x = r->boundary[0], y = r->boundary[1];
	RectReal d_square = (cx - x) * (cx - x) + (cy - y) * (cy - y);
	double cmp = d_square - (cur_d * cur_d);
	if (r->is_use) { /**< R-Tree 상에 데이터가 존재하는 지 여부 확인 */
		if (cmp < 0 || fabs(cmp) < EPSILON) {
			if (d_square > max_d_square) {
				max_id = id;
				max_d_square = d_square;
			} else if (fabs(d_square - max_d_square) < EPSILON) {
				max_id = (max_id > id) ? id : max_id;
				max_d_square = d_square;
			}
			nhits++;
		}
	}

	return 1; // keep going
}

int main(void)
{
	struct Node *root = RTreeNewIndex();
	FILE *fin = NULL;
	FILE *fout = NULL;

	rect_tbl = (struct Rect *)calloc(MAX_TABLE_SIZE, sizeof(struct Rect));
	if (!rect_tbl) {
		fprintf(stderr, "cannot allocate the memory to 'rect_tbl'\n");
		goto exception;
	}

	fin = fopen("pin.txt", "r");
	if (!fin) {
		fprintf(stderr, "'pin.txt' open failed\n");
		goto exception;
	}
	fout = fopen("pout.txt", "w");
	if (!fout) {
		fprintf(stderr, "'pout.txt' open failed\n");
		goto exception;
	}

	while (!feof(fin)) {
		struct Rect rect;
		char cmd;
		long id;

		fscanf(fin, "%c", &cmd);
		switch (cmd) {
		case INSERT:
			fscanf(fin, " %ld", &id);

			fscanf(fin, " %lf %lf\n", &rect.boundary[0],
			       &rect.boundary[1]);

			/**
			 * @brief 점이기 때문에 xmax == xmin이고, ymax == ymin이다.
			 */
			rect.boundary[2] = rect.boundary[0];
			rect.boundary[3] = rect.boundary[1];
			rect_tbl[id] = rect;
			rect_tbl[id].is_use = true;
			RTreeInsertRect(&rect_tbl[id], id, &root, 0);
			break;
		case ERASE:
			fscanf(fin, " %ld\n", &id);
			if (!rect_tbl[id].is_use) {
				break;
			}
			RTreeDeleteRect(&rect_tbl[id], id, &root);
			rect_tbl[id] =
				(struct Rect){ .is_use = false,
					       .boundary = { 0, 0, 0, 0 } };
			break;
		case SEARCH:
			fscanf(fin, " %lf %lf", &cx, &cy);
			fscanf(fin, " %lf\n", &cur_d);

			/**
			 * @brief 원을 감싸는 사각형을 그리도록 한다.
			 */
			rect.boundary[0] = cx - cur_d;
			rect.boundary[1] = cy - cur_d;
			rect.boundary[2] = cx + cur_d;
			rect.boundary[3] = cy + cur_d;

			/**
			 * @brief 전역 변수를 초기화 합니다.
			 */
			nhits = 0;
			max_d_square = -1;
			max_id = -1;
			RTreeSearch(root, &rect, SearchCallback, 0);
			fprintf(fout, "%ld", nhits);
			if (nhits == 0) {
				fprintf(fout, "\n");
			} else {
				fprintf(fout, " %ld\n", max_id);
			}

			break;
		default:
			fprintf(stderr, "invalid command\n");
			goto exception;
		}
	}
	free(rect_tbl);
	fclose(fin);
	fclose(fout);
	return 0;

exception:
	if (!rect_tbl) {
		free(rect_tbl);
	}
	if (!fin) {
		fclose(fin);
	}
	if (!fout) {
		fclose(fout);
	}
	return -1;
}
