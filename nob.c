#define NOB_IMPLEMENTATION
#include "nob.h"
#define FLAG_IMPLEMENTATION
#include "flag.h"

static void usage(void)
{
    fprintf(stderr, "Usage: %s [<FLAGS>] [--] [<program args>]\n", flag_program_name());
    fprintf(stderr, "FLAGS:\n");
    flag_print_options(stderr);
}

bool build_raylib_exec(Procs *procs, Cmd *cmd, const char *bin_path, const char *src_path)
{
    cmd_append(cmd, "cc");
    cmd_append(cmd, "-Wall");
    cmd_append(cmd, "-Wextra");
    cmd_append(cmd, "-ggdb");
    cmd_append(cmd, "-I./raylib-5.5_linux_amd64/include/");
    cmd_append(cmd, "-o", bin_path, src_path);
    cmd_append(cmd, "-L./raylib-5.5_linux_amd64/lib/");
    cmd_append(cmd, "-l:libraylib.a");
    cmd_append(cmd, "-lm");
    return cmd_run(cmd, .async = procs);
}

int main(int argc, char **argv)
{
    GO_REBUILD_URSELF(argc, argv);

    Cmd cmd = {0};
    Procs procs = {0};

    bool run = false;
    bool help = false;
    flag_bool_var(&run, "run", false, "Run the program after compilation.");
    flag_bool_var(&help, "help", false, "Print this help message.");

    if (!flag_parse(argc, argv)) {
        usage();
        flag_print_error(stderr);
        return 1;
    }

    if (help) {
        usage();
        return 0;
    }

    if (!build_raylib_exec(&procs, &cmd, "./dbscan", "dbscan.c")) return 1;
    if (!build_raylib_exec(&procs, &cmd, "./image_segs", "image_segs.c")) return 1;
    if (!procs_flush(&procs)) return 1;

    return 0;
}
