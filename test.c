
#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_TABLE_SIZE ((0x1 << 20) + 1)
#define EPSILON (0.00001)

static struct Rect *rect_tbl;
static RectReal cx, cy;
static RectReal max_d;
static long max_id, nhits;
static RectReal cur_d;

enum { INSERT = '+',
       ERASE = '-',
       SEARCH = '?',
};

int SearchCallback(int id, void *arg)
{
	struct Rect *r = &rect_tbl[id];
	RectReal x = r->boundary[0], y = r->boundary[1];
	RectReal d = (cx - x) * (cx - x) + (cy - y) * (cy - y);
	double cmp = d - (cur_d * cur_d);
	if (r->is_use) {
		if (cmp < 0 || fabs(cmp) < EPSILON) {
			if (d > max_d) {
				max_id = id;
				max_d = d;
			} else if (fabs(d - max_d) < EPSILON) {
				max_id = (max_id > id) ? id : max_id;
				max_d = d;
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

			rect.boundary[0] = cx - cur_d;
			rect.boundary[1] = cy - cur_d;
			rect.boundary[2] = cx + cur_d;
			rect.boundary[3] = cy + cur_d;

			nhits = 0;
			max_d = -1;
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
	// RTreePrintNode(root, 0);
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
