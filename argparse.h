
typedef struct {
    int master_flag_id;
    int *dependant_flag_id;
    int dependantc;
} ReqiuredFlagRule;

typedef struct {
    char *name_long;
    char *name_short;
    char *description;
    int argc;
    int id;
    int disallow_raw_args;
} Flag;

typedef struct {
    int accept_raw_args;
    int flagc;
    Flag *flags;
    char *description;
    ReqiuredFlagRule *rules;
    int rulec;
    char *progname;
} Command;

typedef struct {
    int id;
    int argc;
    char **argv;
} FlagResult;

typedef struct {
    int arg_len;
    char *arg;
} ArgumentResult;

typedef struct {
    char *str;
    int  len;
} RawArg;

typedef struct {
    int raw_argc;
    RawArg *raw_args;
    int flagc;
    FlagResult *flags;
    int argc;
    ArgumentResult *args;
} CommandResult;

char *get_flag_name(Command *cmd, int flag_id);

// Error handling is done inside of this function,
// so the consumer can assume that the resultant values
// (while not necessarily correct) are possible within
// the defined command structure
CommandResult *parse_command(Command *cmd, int argc, char *argv[]);
