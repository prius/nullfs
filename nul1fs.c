/*
    FUSE: Filesystem in Userspace
    Copyright (C) 2001-2005  Miklos Szeredi <miklos@szeredi.hu>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.
*/

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26
#include <sys/stat.h>
#include <time.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

time_t start_t;

// Max path length from
// http://serverfault.com/questions/9546/filename-length-limits-on-linux
const unsigned int PATH_LENGTH = 4096;
char tmp_root[] = "/tmp/fuse-nul1fs";
unsigned int MYPID = -1;

static int strendswith(const char *str, const char *sfx) {
    size_t sfx_len = strlen(sfx);
    size_t str_len = strlen(str);
    if (str_len < sfx_len) return 0;
    return (strncmp(str + (str_len - sfx_len), sfx, sfx_len) == 0);
};

static int nullfs_isdir(const char *path) {
    if (! path) return 0;
    return (strendswith(path, "/") || strendswith(path, "/..")
        || strendswith(path, "/.") || (strcmp(path, "..") == 0)
        || (strcmp(path, ".") == 0));
};

static int nullfs_getattr(const char *path, struct stat *stbuf) {
    int res = -ENOENT;
    if (! path) return -ENOENT;

    memset(stbuf, 0, sizeof(struct stat));

    if (nullfs_isdir(path)) {
        stbuf->st_mode = S_IFDIR | 0777;
        stbuf->st_nlink = 2;
        stbuf->st_atime = time(NULL);
        stbuf->st_mtime = start_t;
        stbuf->st_ctime = start_t;

        res = 0;
    } else {
        char tmp_obj[PATH_LENGTH];

        snprintf(tmp_obj, sizeof tmp_obj, "%s/%d%s", tmp_root, MYPID, path);
        
        struct stat sb;
        if (stat(tmp_obj, &sb) == 0 && S_ISDIR(sb.st_mode)) {
            stbuf->st_mode = S_IFDIR | 0777;
            stbuf->st_nlink = 2;
            stbuf->st_atime = time(NULL);
            stbuf->st_mtime = start_t;
            stbuf->st_ctime = start_t;

            res = 0;
        } else if (S_ISREG(sb.st_mode)) {
            unlink(tmp_obj);
            
            stbuf->st_mode = S_IFREG | 0666;
            stbuf->st_nlink = 1;
            stbuf->st_size = 0;
            stbuf->st_atime = time(NULL);
            stbuf->st_mtime = time(NULL);
            stbuf->st_ctime = time(NULL);
            
            res = 0;
        } else {
            res = -ENOENT;
	};

    };
    return res;
};

static int nullfs_readdir(const char *path, void *buf, fuse_fill_dir_t
filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    if (! nullfs_isdir(path)) return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    return 0;
};

static int nullfs_open(const char *path, struct fuse_file_info *fi) {
    (void) fi;

    if (nullfs_isdir(path)) return -ENOENT;

    return 0;
};

static int nullfs_read(const char *path, char *buf, size_t size,
off_t offset, struct fuse_file_info *fi) {
    (void) buf;
    (void) size;
    (void) offset;
    (void) fi;

    if (nullfs_isdir(path)) return -ENOENT;

    return 0;
};

static int nullfs_write(const char *path, const char *buf, size_t size,
off_t offset, struct fuse_file_info *fi) {
    (void) buf;
    (void) offset;
    (void) fi;

    if (nullfs_isdir(path)) return -ENOENT;

    return (int) size;
};

static int nullfs_create(const char *path, mode_t m,
struct fuse_file_info *fi) {
    (void) path;
    (void) m;
    (void) fi;
    
    char tmp_file[PATH_LENGTH];
    
    snprintf(tmp_file, sizeof tmp_file, "%s/%d%s", tmp_root, MYPID, path);
    
    // Create temporary file
    FILE *fp;
    fp = fopen(tmp_file, "w");
    fclose(fp);
    
    return 0;
};

static int nullfs_mkdir(const char *path, mode_t mode)
{
    (void) path;
    (void) mode;

    char tmp_dir[PATH_LENGTH];

    snprintf(tmp_dir, sizeof tmp_dir, "%s/%d%s", tmp_root, MYPID, path);

    // Create directory for temporary file
    return mkdir(tmp_dir, S_IRWXU | S_IRWXG | S_IRWXO);
}

static int nullfs_unlink(const char *path) {
    (void) path;

    return 0;
};

static int nullfs_rename(const char *src, const char *dst) {
    (void) src;
    (void) dst;

    return 0;
};

static int nullfs_truncate(const char *path, off_t o) {
    (void) path;
    (void) o;

    return 0;
};

static int nullfs_chmod(const char *path, mode_t m) {
    (void) path;
    (void) m;

    return 0;
};

static int nullfs_chown(const char *path, uid_t u, gid_t g) {
    (void) path;
    (void) u;
    (void) g;

    return 0;
};

static int nullfs_utimens(const char *path, const struct timespec ts[2]) {
    (void) path;
    (void) ts;

    return 0;
};

static struct fuse_operations nullfs_oper = {
    .getattr    = nullfs_getattr,
    .readdir    = nullfs_readdir,
    .open       = nullfs_open,
    .read       = nullfs_read,
    .write      = nullfs_write,
    .create     = nullfs_create,
    .unlink     = nullfs_unlink,
    .rmdir      = nullfs_unlink,
    .truncate   = nullfs_truncate,
    .rename     = nullfs_rename,
    .chmod      = nullfs_chmod,
    .chown      = nullfs_chown,
    .utimens    = nullfs_utimens,
    .mkdir      = nullfs_mkdir
};

int main(int argc, char *argv[]) {
    start_t = time(NULL);
    MYPID = getpid();

    char tmp_path[PATH_LENGTH];
    char tmp_rm_path[PATH_LENGTH + 7];
    snprintf(tmp_path, sizeof tmp_path, "%s/%d", tmp_root, MYPID);
    snprintf(tmp_rm_path, sizeof tmp_rm_path, "rm -fr %s/%d", tmp_root, MYPID);

    umask(0000);

    mkdir(tmp_root, S_IRWXU | S_IRWXG | S_IRWXO);
    system(tmp_rm_path);
    mkdir(tmp_path, S_IRWXU | S_IRWXG | S_IRWXO);

    return fuse_main(argc, argv, &nullfs_oper, NULL);
};

/* vi:set sw=4 et tw=72: */
