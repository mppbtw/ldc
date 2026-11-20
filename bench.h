#include "assembler.h"
#include <stdbool.h>

uint64_t run_dts_bench(MachineState m);
int run_dts_item(MachineState m, int num1, int num2);
bool run_full_check(MachineState m);
bool run_single_check(MachineState m, int i, int j);

#define BENCH_CYCLE_LIMIT 50000
