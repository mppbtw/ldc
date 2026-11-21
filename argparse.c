#include "argparse.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

char *get_flag_name(Command *cmd, int flag_id) {
    for (int i=0; i<cmd->flagc; i++) {
        if (flag_id == cmd->flags[i].id) {
            return cmd->flags[i].name_long;
        }
    }
    return NULL;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void print_help(Command *cmd) {
    printf("%s\n", cmd->progname);
    printf("%s\n\n", cmd->description);
    // help menu's flag explainers look like :
    // -h --help    show this help screen
    // -b --build   build many source files into one
    // -I --include include more source files into a build
    //
    // note that the number of spaces between the flags
    // and the descriptions are aligned such that each
    // description is aligned vertically. The following code
    // performs this alignment.

    uint8_t flag_pair_lengths[cmd->flagc];
    uint8_t longest_flag_pair = 0;
    for (int i=0; i<cmd->flagc; i++) {
        uint8_t pair_len = strlen(cmd->flags[i].name_short)
            + strlen(cmd->flags[i].name_long) + 1;
        flag_pair_lengths[i] = pair_len;
        if (pair_len > longest_flag_pair)
            longest_flag_pair = pair_len + 2;
    }
    char *help_str = " -h --help";
    longest_flag_pair = MAX(longest_flag_pair, strlen(help_str)); 
    printf("%s", help_str);
    int padding_len = longest_flag_pair - strlen(help_str);

    for (int j=0; j<padding_len+1; j++)
        printf(" ");
    printf(" Display this information\n");

    for (int i=0; i<cmd->flagc; i++) {
        printf(" %s %s", cmd->flags[i].name_short, cmd->flags[i].name_long);
        uint8_t padding_len = longest_flag_pair - flag_pair_lengths[i];
        for (int j=0; j<padding_len+1; j++)
            printf(" ");
        printf("%s\n", cmd->flags[i].description);
    }
    exit(0);
}

// Error handling is done inside of this function,
// so the consumer can assume that the resultant values
// (while not necessarily correct) are possible within
// the defined command structure
CommandResult *parse_command(Command *cmd, int argc, char *argv[]) {
    CommandResult *res = (CommandResult*) malloc(sizeof(CommandResult));
    // argc is upper bound of the number of flags
    (res->flagc) = 0;
    (res->flags) = (FlagResult *) calloc(argc, sizeof(FlagResult));

    res->argc = 0;
    res->args = (ArgumentResult *) malloc(sizeof(ArgumentResult) * argc);

    res->raw_args = (RawArg *) malloc(sizeof(RawArg) * argc);
    res->raw_argc = 0;

    // Parse flags and arguments
    int flag_args = 1;
    int begin_raw_args = 0;
    for (int i=0; i<argc; i++) {
        if (begin_raw_args) {
            RawArg *raw = (RawArg *) malloc(sizeof(RawArg));
            raw->len = strlen(argv[i]);
            raw->str = argv[i];
            res->raw_args[res->raw_argc] = *raw;
            res->raw_argc++;
            continue;
        }

        if (flag_args) {
            flag_args--;
            continue;
        }
        int is_flag = 0;
        for (int j=0; j<cmd->flagc; j++) {
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
                print_help(cmd);

            if (strcmp(argv[i], cmd->flags[j].name_long) == 0
                    || strcmp(argv[i], cmd->flags[j].name_short) == 0) {
                is_flag = 1;

                // Parse the arguments for this specific flag
                FlagResult *flag = &res->flags[res->flagc];
                flag->argc = cmd->flags[j].argc;
                flag->id = cmd->flags[j].id;
                flag->argv = NULL;

                if (flag->argc > 0) {
                    flag->argv = malloc(sizeof(char *) * flag->argc);
                    if (!flag->argv) {
                        printf("fatal: memory allocation failed\n");
                        exit(1);
                    }
                }

                for (int k=0; k<cmd->flags[j].argc; k++) {
                    if (i + k + 1 >= argc) {
                        printf("fatal: Flag %s requires at least %d arguments\n",
                                cmd->flags[j].name_long,
                                cmd->flags[j].argc
                        );
                        exit(1);
                    }
                    flag_args++;
                    flag->argv[k] = argv[i+1+k];
                }




                res->flagc++;

            }
        };

        if (!is_flag) {
            if (cmd->accept_raw_args && (strcmp(argv[i], "--") == 0)) {
                begin_raw_args = 1;
                continue;
            }
            res->args[res->argc].arg = argv[i];
            res->args[res->argc].arg_len = strlen(argv[i]);
            res->argc++;
        }
    };

    if (res->raw_argc > 0) {
        for (int res_flag=0; res_flag<res->flagc; res_flag++) {
            for (int cmd_flag=0; cmd_flag<cmd->flagc; cmd_flag++) {
                if ( res->flags[res_flag].id == cmd->flags[cmd_flag].id
                        && cmd->flags[cmd_flag].disallow_raw_args) {

                    printf("fatal: flag '%s' does not allow raw arguments with '--'",
                            get_flag_name(cmd, res->flags[res_flag].id));
                    printf(" but %d were found\n", res->raw_argc);
                    exit(1);
                }
            }
        }
    }

    for (int flag=0; flag<res->flagc; flag++) {
        for (int arg=0; arg<res->flags[flag].argc; arg++) {
            //printf("FLAG ARG: %s\n", res->flags[flag].argv[arg]);
        }
    }

    // Enforce ReqiuredFlagRules
    for (int rule=0; rule<cmd->rulec; rule++) {
        // Check if master is there
        int master_present = 0;
        for (int flag=0; flag<res->flagc; flag++) {
            if (res->flags[flag].id == cmd->rules[rule].master_flag_id) {
                master_present = 1;
                break;
            }
        }

        if (master_present)
            continue;

        // It is now known that the master flag for this rule is not present,
        // so any dependant flags being present is a violation of the rule
        for (int flag=0; flag<res->flagc; flag++) {
            for (int rflag=0; rflag<cmd->rules[rule].dependantc; rflag++) {
                if (res->flags[flag].id == cmd->rules[rule].dependant_flag_id[rflag]) {
                    printf("fatal: flag '%s' must be used with '%s'\n",
                            get_flag_name(cmd, res->flags[flag].id),
                            get_flag_name(cmd, cmd->rules[rule].master_flag_id));
                    exit(1);
                }
            }
        }
    }
    return res;
}
