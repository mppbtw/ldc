#include <locale.h>
#include <omp.h>
#include <time.h>
#include <inttypes.h>
#include "bench.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "argparse.h"
#include "global.h"
#include "assembler.h"
#include "run.h"

#ifdef _WIN32
    #include <windows.h>
    void usleep(unsigned int us) {
        Sleep((us+999)/1000); // rounds up to the nearest ms since
                              // windows cant do microseconds
    }
#else
    #include <unistd.h>
#endif

typedef struct {
    char *source_path;
    char *contents;
    char *fname;
} SrcFile;

#define BART  "      |\\/\\/\\/|\n" \
"      |      |\n" \
"      |      |\n" \
"      | (o)(o)\n" \
"      C      _)      there is work to be done brother\n" \
"       | ,___|\n" \
"       |   /\n" \
"      /____\\\n" \
"     /      \\\n"

#define BART_HAPPY \
"                |\\|\\,'\\,'\\ ,.\n" \
"            )        ;' |,'\n" \
"           /              |,'|,.\n" \
"          /                  ` /__\n" \
"         ,'                    ,-'\n" \
"        ,'                    :\n" \
"       (_                     '\n" \
"     ,'                      ;\n" \
"     |---._ ,'     .        '\n" \
"     :   o Y---.__  ;      ;\n" \
"     /`,""-|     o`.|     /              GREAT JOB BROTHER!\n" \
"    ,  `._  `.    ,'     ;\n" \
"    ;         `""'      ;\n" \
"   /                   -'.\n" \
"   \\                   G  )\n" \
"    `-.__________,   `._,'\n" \
"            (`   `     |)\\\n" \
"    .      / `.       ,'  \\\n" \
"          /    `-----'     \\\n" \
"         /\n" \




#define ldc_print(...) do {\
            fprintf(stdout, "%s(CPU output):  %s", TERMINAL_CYAN, TERMINAL_COLOR_RESET); \
            fprintf(stdout, __VA_ARGS__);} while (0)\

#define ldc_ask(...)\
            fprintf(stdout, "%s!!!%s  %s(CPU requests input):  %s",\
                    TERMINAL_BOLD,\
                    TERMINAL_COLOR_RESET,\
                    TERMINAL_CYAN, TERMINAL_COLOR_RESET); \

#define printstate(m, i) {char *r = render_machine(m, i);\
                                   printf("%s", r);\
                                   fflush(stdout);\
                                   free(r);}\

#define DEFAULT_CODE "start LDA wow\n\
    OUT\n\
    SUB ONE\n\
    STO wow\n\
    BRP start\n\
HLT\n\
wow DAT 10\n\
ONE DAT 1\n"

#define DEFAULT_DCL_PERIOD 50000

void print_version() {
    printf("0.0.0\n");
};

void print_commas(uint64_t n) {
    if (n >= 1000) {
        print_commas(n / 1000);
        printf(",%03llu", (unsigned long long)(n % 1000));
    } else {
        printf("%llu", (unsigned long long)n);
    }
}

enum FlagIds {
    VERSION_ID,
    VERBOSE_ID,
    FREQ_ID,
    BENCHMARK_ID,
    BENCHMARK_FULL_ID,
    THREADS_ID,
    CHECK_ID
};

bool verbose_enabled = false;
bool clock_enabled = true;
int benchmarking = 0;

int main(int argc, char *argv[]) {
    Command *cmd = (Command*) malloc(sizeof(Command));
    cmd->description = "Ludicrous Difficulty Computer is a "
        TERMINAL_ITALIC "blazing fast" TERMINAL_COLOR_RESET" interpreter for \n"
        "Little Man/Minion Computer which includes featured designed to test \n"
        "submissions for the Computer Systems Coursework.\n\n"
        "In no way is LDC capable of assisting users with the creation of their \n"
        "LMC programs, nor does it include any coursework solutions itself.";
    cmd->progname = "ldc";
    cmd->accept_raw_args = 0;
    cmd->flagc = 7;
    cmd->flags = (Flag*) malloc(sizeof(Flag) * (cmd->flagc));
    cmd->rulec = 0;

    cmd->flags[0].name_long = "--version";
    cmd->flags[0].name_short = "-V";
    cmd->flags[0].id = VERSION_ID;
    cmd->flags[0].argc = 0;
    cmd->flags[0].description = "Print the current build date, commit hash and version";

    cmd->flags[1].name_long = "--verbose";
    cmd->flags[1].name_short = "-v";
    cmd->flags[1].argc = 0;
    cmd->flags[1].id = VERBOSE_ID;
    cmd->flags[1].description = "Display extra info";

    cmd->flags[2].name_long = "--deblequency";
    cmd->flags[2].name_short = "-d";
    cmd->flags[2].argc = 1;
    cmd->flags[2].id = FREQ_ID;
    cmd->flags[2].description = "Sets the clock deblequency at which the CPU runs";

    cmd->flags[3].name_long = "--benchmark";
    cmd->flags[3].name_short = "-b";
    cmd->flags[3].argc = 0;
    cmd->flags[3].id = BENCHMARK_ID;
    cmd->flags[3].description = "Benchmarks the computer systems coursework";

    cmd->flags[4].name_long = "--benchmark-full";
    cmd->flags[4].name_short = "-B";
    cmd->flags[4].argc = 0;
    cmd->flags[4].id = BENCHMARK_FULL_ID;
    cmd->flags[4].description = "Benchmarks the computer systems coursework on all 1,000,000 possible inputs";

    cmd->flags[5].name_long = "--threads";
    cmd->flags[5].name_short = "-t";
    cmd->flags[5].argc = 1;
    cmd->flags[5].id = THREADS_ID;
    cmd->flags[5].description = "Sets how many threads to be used when benchmarking. Defaults to 1";

    cmd->flags[6].name_long = "--check";
    cmd->flags[6].name_short = "-c";
    cmd->flags[6].argc = 0;
    cmd->flags[6].id = CHECK_ID;
    cmd->flags[6].description = "Checks the computer systems coursework for validity with all possible inputs";



    CommandResult *res =  parse_command(cmd, argc, argv);

    omp_set_num_threads(1);

    // The presence of the verbose flag can affect how the
    // other flags are handled, as they may contain -v flags.
    // For this reason verbose is checked for first
    for (int i=0; i<res->flagc; i++) {
        if (res->flags[i].id == VERBOSE_ID) {
            verbose_enabled = true;
            dbg("Verbose mode has been enabled\n");
        }
    }

    bool custom_clock_period = false;
    int clock_period = DEFAULT_DCL_PERIOD;
    for (int i=0; i<res->flagc; i++) {
        switch (res->flags[i].id) {
            case VERSION_ID:
                print_version();
                exit(0);
                break;
            case BENCHMARK_FULL_ID:
                benchmarking = 2;
                dbg("Disabling the Debleena Clock Limiter™ (DCL)\n");
                clock_enabled = false;
                break;
            case BENCHMARK_ID:
                benchmarking = 1;
                dbg("Disabling the Debleena Clock Limiter™ (DCL)\n");
                clock_enabled = false;
                break;
            case CHECK_ID:
                benchmarking = 3;
                break;
            case THREADS_ID:
                ; char *thread_str = res->flags[i].argv[0];
                int threads = atoi(thread_str);
                if (threads == 0) {
                    errprint("Invalid thread count: '%s'\n", thread_str);
                    exit(1);
                }
                dbg("Setting benchmarking thread count to %d\n", threads);
                omp_set_num_threads(threads);
                break;
            case FREQ_ID:
                custom_clock_period = true;
                ; char *freq_str = res->flags[i].argv[0];
                if (!strcmp(freq_str, "inf")) {
                    dbg("Disabling the Debleena Clock Limiter™ (DCL)\n");
                    clock_enabled = false;
                    break;
                }

                char *endptr;
                errno = 0;
                double freq = (double) strtod(freq_str, &endptr);
                if (errno || freq_str == endptr || (*endptr)) {
                    errprint("Invalid deblequency: %s\n", freq_str);
                    exit(1);
                }
                if (freq >= 10000) {
                    hint("It appears your'e using a high deblequency value, use 'inf'"
                         "as the deblequency to disable the Debleena Clock Limeter™ (DCL)\n");
                }
                clock_period = 1000000 / freq;

                dbg(
                        "Setting Debleena Clock Limiter™ (DCL) "
                        "to deblequency %fHz, period: %dµs\n",
                        freq, clock_period);

                break;
        }
    }

    if (!custom_clock_period && clock_enabled) {
        dbg("Debleena Clock Limiter™ (DCL) was not given a value. Defaulting to %d\n", DEFAULT_DCL_PERIOD);
    }


#ifdef __linux__
#include <sys/resource.h>
    if (benchmarking) {
        if (setpriority(PRIO_PROCESS, 0, -20) == -1) {
            hint("Failed to assign maximum thread priority to this process."
                    " Running the benchmark as root may increase processing speed.\n");
        } else {
            printf("Set the process priority to -20\n");
        }
    }

#endif

    if (res->argc > 1) {
        errprint("Too many arguments specified\n");
        exit(1);
    }

    if (res->argc == 0) {
        errprint("No source files specified. Type -h for help\n");
        exit(1);
    }

    char *code = DEFAULT_CODE;
    char *src = res->args[0].arg;
    dbg("Source file: '%s'\n", src);

    FILE *fptr = fopen(src, "r");

    if (fptr == 0) {
        errprint("Failed to open file: '%s': \n", strerror(errno));
        exit(1);
    }

    dbg("Opened source file with fd=%d\n", fileno(fptr));

    fseek(fptr, 0, SEEK_END);
    int buf_size = ftell(fptr);

    dbg("Reading source file into a %d byte buffer\n", buf_size);

    fseek(fptr, 0, SEEK_SET);
    if (!buf_size) {
        errprint("Failed to read file: %s", strerror(errno));
        exit(1);
    }

    char *buf = malloc(buf_size+10);
    memset(buf, 0, buf_size+10);
    fread(buf, 1, buf_size, fptr);

    dbg("Successfully read %s into memory\n", src);

    
    int err;
    MachineState m = assemble(buf, &err);
    m.pc = 0;
    m.calculator = 0;
    m.is_neg = false;
    if (err) {
        errprint("LDC has crashed!\n");
        exit(1);
    }

    if (benchmarking == 1) {
        printf("Running a basic benchmark on file %s\n", src);

        clock_t c1 = clock();
        uint64_t cycles = run_dts_item(m, 999, 999);
        float delta_t = (float) (clock() - c1)/CLOCKS_PER_SEC;

        if (cycles == -1) {
            errprint("Benchmark timed out after %d cycles\n", BENCH_CYCLE_LIMIT);
            exit(1);
        }

        setlocale(LC_NUMERIC, "");
        if (delta_t < 1) {
            if (delta_t < 0.001) {
                printf("Finished basic benchmark in %gµs\n", delta_t*1000000);
            } else {
                printf("Finished basic benchmark in %gms\n", delta_t*1000);
            }
        } else{
            printf("Finished basic benchmark in %gs\n", delta_t);
        }
        printf("Your program ran in " TERMINAL_BOLD_INTENSE TERMINAL_ULINE);
        print_commas(cycles);
        printf(TERMINAL_COLOR_RESET " clock cycles\n");
        exit(0);
    }

    if (benchmarking == 2) {
        printf("Running a full benchmark on file %s\n", src);
        struct timespec t1, t2;

        // monotonic clock is shared between each thread
        clock_gettime(CLOCK_MONOTONIC, &t1);

        clock_t c1 = clock();
        uint64_t cycles = run_dts_bench(m);
        clock_gettime(CLOCK_MONOTONIC, &t2);

        double delta_t = ((double) (clock() - c1))/CLOCKS_PER_SEC;
        double delta_t_monotonic = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1e9;


        if (delta_t < 1) {
            if (delta_t < 0.001) {
                printf("Finished basic benchmark in %gµs\n", delta_t_monotonic*1000000);
                printf("Combined CPU time: %f\n", delta_t*1000000);
            } else {
                printf("Finished basic benchmark in %gms\n", delta_t_monotonic*1000);
                printf("Combined CPU time: %f\n", delta_t*1000);
            }
        } else{
            printf("Finished basic benchmark in %gs\n", delta_t_monotonic);
            printf("Combined CPU time: %f\n", delta_t);
        }


        if (cycles == -1) {
            errprint("Benchmark timed out after %d cycles\n", BENCH_CYCLE_LIMIT);
            exit(1);
        }

        printf("Your program ran in " TERMINAL_BOLD_INTENSE TERMINAL_ULINE);
        print_commas(cycles);
        printf(TERMINAL_COLOR_RESET " clock cycles\n");

        printf("This gives an average of %lu cycles per test\n", cycles/1000000);
        exit(0);
    }

    if (benchmarking == 3) {
        printf("Running a full check on file '%s'\n", src);
        struct timespec t1, t2;

        // monotonic clock is shared between each thread
        clock_gettime(CLOCK_MONOTONIC, &t1);

        clock_t c1 = clock();
        bool result = run_full_check(m);
        clock_gettime(CLOCK_MONOTONIC, &t2);

        double delta_t = ((double) (clock() - c1))/CLOCKS_PER_SEC;
        double delta_t_monotonic = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1e9;
        printf("Finished full check in %f seconds\n", delta_t_monotonic);
        printf("Combined CPU time: %f\n", delta_t);
        if (result) {
            printf("%sAll 1,000,000 tests passed%s\n", TERMINAL_GREEN, TERMINAL_COLOR_RESET);
            printf("%s", BART_HAPPY);
        } else {
            printf("%s%s failed the test suite%s\n", TERMINAL_RED, src, TERMINAL_COLOR_RESET);
            printf("%s", BART);
        }
        exit(0);
    }

    ExtraRenderInfo i;
    memset(&i.last_used, 255, sizeof(i.last_used));
    printstate(&m, i);

    int output = 0;
    int last_input = 0;
    int cycles = 0;
    int output_counter = 0;

    while (1) {
        cycles++;
        clock_t c1 = clock();
        int r = step_machine(&m, &output, last_input);
        if (r == 4)
            break;

        if (r == 1) {
            ldc_print("%d\n", output);
            output_counter++;
        }

        if (r == 2) {
            char inpbuf[1024];
            while (1) {
                ldc_ask();
                if (!fgets(buf, 1024, stdin))
                    return 1;

                char *endptr;
                errno = 0;
                last_input = strtol(buf, &endptr, 10);
                if (!errno && buf != endptr && (!*endptr || *endptr == '\n')) {
                    break;
                }
            }
        }

        if (r == 3) {
            errprint("The interpreter encountered an error (likely out of bounds)\n");
            exit(1);
        }

        if (r == 5)
            printstate(&m, i);

        output = 0;


        if (clock_enabled) {
            long elapsed_us;
            if (CLOCKS_PER_SEC == 1000000) {
                long elapsed_us = (long) (clock()-c1);
            } else if (CLOCKS_PER_SEC == 1000) {
                long elapsed_us = (long) (clock()-c1) * 1000;
            } else {
                long elapsed_us = (long) (clock()-c1) * 1000000/CLOCKS_PER_SEC;
            }
            usleep(clock_period-elapsed_us);
        }
    }
    printstate(&m, i);
    dbg("The program ran in %d cycles!\n", cycles);
    dbg("The program outputted %d times\n", output_counter);
}
