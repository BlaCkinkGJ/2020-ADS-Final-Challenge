
#include "index.h"

#define METHODS 1

struct Branch BranchBuf[MAXCARD + 1];
int BranchCount;
struct Rect CoverSplit;

/**
 * @brief 파티션을 찾는 변수입니다.
 */
struct PartitionVars {
	int partition[MAXCARD + 1];
	int total, minfill;
	int taken[MAXCARD + 1];
	int count[2];
	struct Rect cover[2];
	RectReal area[2];
} Partitions[METHODS];
