#include "bench.h"
#include "omp.h"
#include "assembler.h"
#include "run.h"
#include "global.h"
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int run_dts_item(MachineState m, int num1, int num2) {
    int cycles = 0;
    int output = 0;


    int which_input = 0;
    while (1) {
        cycles++;

        if (cycles > BENCH_CYCLE_LIMIT) {
            return -1;
        }

        int r;
        switch (which_input) {
            case 0:
                r = step_machine(&m, &output, 0);
                break;
            case 1:
                r = step_machine(&m, &output, num1);
                break;
            case 2:
                r = step_machine(&m, &output, num2);
                break;
        }

        if (r == 4)
            break;

        if (r == 2) {
            which_input++;
            if (which_input > 2) {
                errprint("Program requested too many inputs.");
                exit(1);
            }
        }

    }
    return cycles;
}

uint64_t run_dts_bench(MachineState m) {
    uint64_t total_c = 0;
    bool timed_out = false;

#pragma omp parallel for reduction(+:total_c)
    for (int i=0; i<1000*1000; i++) {
        int res = run_dts_item(m, i/1000, i % 1000);
        if (res == -1) {
            i = 1000*1000 + 1;
            timed_out = true;
        }
        total_c += res;
    }

    if (timed_out) {
        return -1;
    }
    return total_c;
}

int expected_vals(int num1, int num2, int *expect) {
    expect[0] = num1;
    expect[1] = num2;

    if (num1 == 0 && num2 == 0)
        return 2;

    int i = 2;
    while (1) {
        num1 = num1 - (num1/100) - ((num1%100)/10) - (num1%10);
        expect[i] = num1;
        i++;
        num2 = num2 - (num2/100) - ((num2%100)/10) - (num2%10);
        expect[i] = num2;
        i++;
        if (num1 == 0 && num2 == 0) {
            return i;
        }
    }
};


bool run_single_check(MachineState m, int i, int j) {
    int output = 0;
    int expect[256];
    memset(expect, 0, sizeof(expect));

    int expected_output_count = expected_vals(i, j, expect);
    int expect_i = 0;

    int which_input = 0;
    while (1) {
        int r;
        switch (which_input) {
            case 0:
                r = step_machine(&m, &output, 0);
                break;
            case 1:
                r = step_machine(&m, &output, i);
                break;
            case 2:
                r = step_machine(&m, &output, j);
                break;
        }

        if (r == 1) {
            if (expect[expect_i] != output) {
                pthread_mutex_lock(&mutex);
                dbg("Detected a failed test: expected %d, got %d, "
                    "num1=%d, num2=%d\n", expect[expect_i], output, i, j);
                pthread_mutex_unlock(&mutex);
                return false;
            }
            expect_i++;
        }

        if (r == 4)
            break;

        if (r == 2) {
            which_input++;
            if (which_input > 2) {
                pthread_mutex_lock(&mutex);
                dbg("Detected a failed test, num1=%d, num2=%d; "
                    "Program requested too many inputs", i, j);
                pthread_mutex_unlock(&mutex);
                return false;
            }
        }

    }
    if (expected_output_count != expect_i) {
        pthread_mutex_lock(&mutex);
        dbg("Detected a failed test, num1=%d, num2=%d; "
            "Program procuded %d outputs, expected %d\n", i, j, expect_i, expected_output_count);
        pthread_mutex_unlock(&mutex);
        return false;
    }
    return true;
}


bool run_full_check(MachineState m) {

    volatile bool result = true;;
#pragma omp parallel for shared(result)
    for (int i=0; i<1000*1000; i++) {

        if (!run_single_check(m, i/1000, i % 1000)) {
            result = false;
            i = 1000*1000 + 1; // place i out of bounds so as
                               // to terminate the spawning
                               // of new threads.
        }

    }

    return result;
}
