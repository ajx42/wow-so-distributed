#define FUSE_USE_VERSION 29

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#include <fuse.h>

#include "unreliablefs_ops.h"
#include "unreliablefs.h"

extern struct err_inj_q *config_init(const char* conf_path);
extern void config_delete(struct err_inj_q *config);

struct unreliablefs_config conf;

static struct fuse_operations unreliable_ops;

enum {
     KEY_HELP,
     KEY_VERSION,
     KEY_DEBUG,
};

#define UNRELIABLEFS_OPT(t, p, v) { t, offsetof(struct unreliablefs_config, p), v }
#define UNRELIABLEFS_VERSION "0.1"

static struct fuse_opt unreliablefs_opts[] = {
    UNRELIABLEFS_OPT("-seed=%u",           seed, 0),
    UNRELIABLEFS_OPT("-basedir=%s",        basedir, 0),
    UNRELIABLEFS_OPT("-server_address=%s", server_address, 0),

    FUSE_OPT_KEY("-d",             KEY_DEBUG),
    FUSE_OPT_KEY("-V",             KEY_VERSION),
    FUSE_OPT_KEY("-v",             KEY_VERSION),
    FUSE_OPT_KEY("--version",      KEY_VERSION),
    FUSE_OPT_KEY("-h",             KEY_HELP),
    FUSE_OPT_KEY("--help",         KEY_HELP),
    FUSE_OPT_KEY("subdir",         FUSE_OPT_KEY_DISCARD),
    FUSE_OPT_KEY("modules=",       FUSE_OPT_KEY_DISCARD),
    FUSE_OPT_END
};

static int unreliablefs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
    switch (key) {
    case KEY_HELP:
        fprintf(stderr,
            "usage: unreliablefs mountpoint [options]\n\n"
            "general options:\n"
            "    -h   --help            print help\n"
            "    -v   --version         print version\n"
            "    -d                     enable debug output (implies -f)\n"
            "    -f                     foreground operation\n\n"
            "unreliablefs options:\n"
            "    -seed=NUM              random seed\n"
            "    -basedir=STRING        directory to mount\n\n");
        exit(1);

    case KEY_VERSION:
        fprintf(stderr, "unreliablefs version %s\n", UNRELIABLEFS_VERSION);
        fuse_opt_add_arg(outargs, "--version");
        fuse_main(outargs->argc, outargs->argv, &unreliable_ops, NULL);
        exit(1);
    }
    return 1;
}

int is_dir(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }

    return S_ISDIR(statbuf.st_mode);
}

void setup_unreliable_ops(){
    unreliable_ops.getattr     = unreliable_getattr,
    unreliable_ops.readlink    = unreliable_readlink,
    unreliable_ops.mknod       = unreliable_mknod,
    unreliable_ops.mkdir       = unreliable_mkdir,
    unreliable_ops.unlink      = unreliable_unlink,
    unreliable_ops.rmdir       = unreliable_rmdir,
    unreliable_ops.symlink     = unreliable_symlink,
    unreliable_ops.rename      = unreliable_rename,
    unreliable_ops.link        = unreliable_link,
    unreliable_ops.chmod       = unreliable_chmod,
    unreliable_ops.chown       = unreliable_chown,
    unreliable_ops.truncate    = unreliable_truncate,
    unreliable_ops.open	    = unreliable_open,
    unreliable_ops.read	    = unreliable_read,
    unreliable_ops.write       = unreliable_write,
    unreliable_ops.statfs      = unreliable_statfs,
    unreliable_ops.flush       = unreliable_flush,
    unreliable_ops.release     = unreliable_release,
    unreliable_ops.fsync       = unreliable_fsync,
#ifdef HAVE_XATTR
    unreliable_ops.setxattr    = unreliable_setxattr,
    unreliable_ops.getxattr    = unreliable_getxattr,
    unreliable_ops.listxattr   = unreliable_listxattr,
    unreliable_ops.removexattr = unreliable_removexattr,
#endif /* HAVE_XATTR */
    unreliable_ops.opendir     = unreliable_opendir,
    unreliable_ops.readdir     = unreliable_readdir,
    unreliable_ops.releasedir  = unreliable_releasedir,
    unreliable_ops.fsyncdir    = unreliable_fsyncdir,

    unreliable_ops.init        = unreliable_init,
    unreliable_ops.destroy     = unreliable_destroy,

    unreliable_ops.access      = unreliable_access,
    unreliable_ops.create      = unreliable_create,
    unreliable_ops.ftruncate   = unreliable_ftruncate,
    unreliable_ops.fgetattr    = unreliable_fgetattr,
    unreliable_ops.lock        = unreliable_lock,
#if !defined(__OpenBSD__)
    unreliable_ops.ioctl       = unreliable_ioctl,
#endif /* __OpenBSD__ */
#ifdef HAVE_FLOCK
    unreliable_ops.flock       = unreliable_flock,
#endif /* HAVE_FLOCK */
#ifdef HAVE_FALLOCATE
    unreliable_ops.fallocate   = unreliable_fallocate,
#endif /* HAVE_FALLOCATE */
#ifdef HAVE_UTIMENSAT
    unreliable_ops.utimens     = unreliable_utimens;
#endif /* HAVE_UTIMENSAT */
}

int main(int argc, char *argv[])
{
    setup_unreliable_ops();
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    memset(&conf, 0, sizeof(conf));
    conf.seed = time(0);
    conf.basedir = (char*)"/";
    conf.server_address = (char*)"localhost:50051";
    fuse_opt_parse(&args, &conf, unreliablefs_opts, unreliablefs_opt_proc);
    srand(conf.seed);
    fprintf(stdout, "random seed = %d\n", conf.seed);

    fprintf(stdout, "server address is = %s\n", conf.server_address);

    if (is_dir(conf.basedir) == 0) {
       fprintf(stderr, "basedir ('%s') is not a directory\n", conf.basedir);
       fuse_opt_free_args(&args);
       return EXIT_FAILURE;
    }
    char subdir_option[PATH_MAX];
    sprintf(subdir_option, "-omodules=subdir,subdir=%s", conf.basedir);
    fuse_opt_add_arg(&args, subdir_option);
    /* build config_path */
    char *real_path = realpath(conf.basedir, NULL);
    if (!real_path) {
        perror("realpath");
        fuse_opt_free_args(&args);
        return EXIT_FAILURE;
    }
    conf.basedir = real_path;
    size_t sz = strlen(DEFAULT_CONF_NAME) + strlen(conf.basedir) + 2;
    conf.config_path = (char*)malloc(sz);
    if (!conf.config_path) {
        perror("malloc");
        fuse_opt_free_args(&args);
        return EXIT_FAILURE;
    }
    /* read configuration file on start */
    snprintf(conf.config_path, sz, "%s/%s", conf.basedir, DEFAULT_CONF_NAME);
    conf.errors = config_init(conf.config_path);
    if (!conf.errors) {
        fprintf(stdout, "error injections are not configured!\n");
    }
    if (pthread_mutex_init(&conf.mutex, NULL) != 0) {
        fuse_opt_free_args(&args);
        perror("pthread_mutex_init");
        return EXIT_FAILURE;
    }

    fprintf(stdout, "starting FUSE filesystem unreliablefs\n");
    int ret = fuse_main(args.argc, args.argv, &unreliable_ops, &conf);

    /* cleanup */
    fuse_opt_free_args(&args);
    config_delete(conf.errors);
    if (conf.config_path)
        free(conf.config_path);
    if (!ret) {
        fprintf(stdout, "random seed = %d\n", conf.seed);
    }

    return ret;
}
