#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/file.h>
#ifdef HAVE_XATTR
#include <sys/xattr.h>
#endif /* HAVE_XATTR */

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#define ERRNO_NOOP -999

#include "unreliablefs_ops.h"
#include <fstream>
#include <string>
#include <vector>

#define WOWFS_LOG_FILE "/tmp/wowfs_local/log"
namespace
{
  void logline(std::string line)
  {
    std::ofstream off("/tmp/logs.unreliable.txt", std::ios_base::app);
    off << std::string(line) << std::endl;
    // ping server to demonstrate
    WowManager::Instance().client.Ping(111);
  }
}

const char *fuse_op_name[] = {
    "getattr",
    "readlink",
    "mknod",
    "mkdir",
    "unlink",
    "rmdir",
    "symlink",
    "rename",
    "link",
    "chmod",
    "chown",
    "truncate",
    "open",
    "read",
    "write",
    "statfs",
    "flush",
    "release",
    "fsync",
#ifdef HAVE_XATTR
    "setxattr",
    "getxattr",
    "listxattr",
    "removexattr",
#endif /* HAVE_XATTR */
    "opendir",
    "readdir",
    "releasedir",
    "fsyncdir",
    "access",
    "creat",
    "ftruncate",
    "fgetattr",
    "lock",
#if !defined(__OpenBSD__)
    "ioctl",
#endif /* __OpenBSD__ */
#ifdef HAVE_FLOCK
    "flock",
#endif /* HAVE_FLOCK */
#ifdef HAVE_FALLOCATE
    "fallocate",
#endif /* HAVE_FALLOCATE */
#ifdef HAVE_UTIMENSAT
    "utimens",
#endif /* HAVE_UTIMENSAT */
    "lstat"
};

extern int error_inject(const char* path, fuse_op operation);


void convert_path(char * file_path)
{
    const char *local_prefix = "/tmp/wowfs_local/";
    const char *remote_prefix = "/tmp/wowfs_remote/";

    if (strncmp(file_path, local_prefix, strlen(local_prefix)) == 0) {
        memmove(file_path + strlen(remote_prefix), file_path + strlen(local_prefix), strlen(file_path) - strlen(local_prefix) + 1);
        memmove(file_path, remote_prefix, strlen(remote_prefix));
    }
}

int unreliable_lstat(const char *path, struct stat *buf)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "lstat %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_LSTAT);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    memset(buf, 0, sizeof(struct stat));
    if (lstat(path, buf) == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_getattr(const char *path, struct stat *buf)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "getattr %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_GETATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }
    
    char converted_path[100];
    strcpy(converted_path, path);
    convert_path(converted_path);

    memset(buf, 0, sizeof(struct stat));
    
    //Send request to server for file stat info.
    RPCResponse response = WowManager::Instance().client.DownloadStat(std::string(converted_path), buf);

    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "getattr recieved\n");
    fprintf(file, "\tgetattr: response %d\n", response.ret_);

    //Verify response 
    if(response.ret_ == -1)
    {
        fprintf(file, "\tgetattr: errno %d\n", response.server_errno_);
        fclose(file);
        return -response.server_errno_;
    }

    fprintf(file, "\tgetattr inode : %lu\n", buf->st_ino);
    fclose(file);

    return 0;
}

int unreliable_readlink(const char *path, char *buf, size_t bufsiz)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "readlink %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_READLINK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = readlink(path, buf, bufsiz);
    if (ret == -1) {
        return -errno;
    }
    buf[ret] = 0;

    return 0;
}

int unreliable_mknod(const char *path, mode_t mode, dev_t dev)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "mknod %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_MKNOD);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = mknod(path, mode, dev);    
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_mkdir(const char *path, mode_t mode)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "mkdir %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_MKDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    char converted_path[100];
    strcpy(converted_path, path);
    convert_path(converted_path);

    // Send request to server to create directory.
    RPCResponse response = WowManager::Instance().client.Mkdir(
        std::string(converted_path), mode);

    file = fopen(WOWFS_LOG_FILE, "a");
    if (response.ret_ == -1) {
        fprintf(file, "\tserver mkdir failed: errno %d\n", response.server_errno_);
        fclose(file);
        return -response.server_errno_;
    }
    fprintf(file, "\tserver mkdir success\n");

    // Create local directory.
    ret = mkdir(path, mode);
    if (ret == -1) {
        fprintf(file, "\tlocal mkdir failed, errno %d\n", errno);
        fclose(file);
        return -errno;
    }
    fprintf(file, "\tlocal mkdir success\n");
    fclose(file);

    return 0;
}

int unreliable_unlink(const char *path)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "unlink %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_UNLINK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = unlink(path); 
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_rmdir(const char *path)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "rmdir %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_RMDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    char converted_path[100];
    strcpy(converted_path, path);
    convert_path(converted_path);

    // Send request to server to remove directory.
    RPCResponse response = WowManager::Instance().client.Rmdir(std::string(converted_path));

    file = fopen(WOWFS_LOG_FILE, "a");
    if (response.ret_ == -1) {
        fprintf(file, "\tserver rmdir failed: errno %d\n", response.server_errno_);
        fclose(file);
        return -response.server_errno_;
    }
    fprintf(file, "\tserver rmdir success\n");

    // Remove local directory.
    ret = rmdir(path);
    if (ret == -1) {
        fprintf(file, "\tlocal rmdir failed, errno %d\n", errno);
        fclose(file);
        return -1;
    }
    fprintf(file, "\tlocal rmdir success\n");
    fclose(file);

    return 0;
}

int unreliable_symlink(const char *target, const char *linkpath)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "symlink %s %s\n", target, linkpath);
    fclose(file);

    int ret = error_inject(target, OP_SYMLINK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = symlink(target, linkpath);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_rename(const char *oldpath, const char *newpath)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "rename %s %s\n", oldpath, newpath);
    fclose(file);

    int ret = error_inject(oldpath, OP_RENAME);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = rename(oldpath, newpath);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_link(const char *oldpath, const char *newpath)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "link %s %s\n", oldpath, newpath);
    fclose(file);

    int ret = error_inject(oldpath, OP_LINK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = link(oldpath, newpath);
    if (ret < 0) {
        return -errno;
    }

    return 0;
}

int unreliable_chmod(const char *path, mode_t mode)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "chmod %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_CHMOD);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }
    
    ret = chmod(path, mode);
    if (ret < 0) {
        return -errno;
    }

    return 0;
}

int unreliable_chown(const char *path, uid_t owner, gid_t group)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "chown %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_CHOWN);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = chown(path, owner, group);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_truncate(const char *path, off_t length)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "truncate %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_TRUNCATE);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = truncate(path, length); 
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_open(const char *path, struct fuse_file_info *fi)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "open %s\n", path);
    fclose(file);
    logline(std::string("wowFS -> open called: ") + std::string(path));
 
    int ret = error_inject(path, OP_OPEN);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    char converted_path[100];
    strcpy(converted_path, path);
    convert_path(converted_path);

    bool fetch = true;

    // check if the file is already available in cache
    // @TODO decide whether to fetch or not
    // for now we always fetch 
    fi->fh = -1;

    struct stat serverStat;
    // we are using the getattr as a way to figure if file exists on the server
    auto res = unreliable_getattr( path, &serverStat );
    if ( res == 0 ) {
      // there is a file on the server, let's pull it
      auto fileSize = serverStat.st_size;
      std::string readBuf;
      auto response = WowManager::Instance().client.DownloadFile(
        std::string(converted_path), readBuf, fileSize);
      if ( response.ret_ == -1 ) {
        // there is a file on the server, but we are not able to read it!
        return -response.server_errno_;
      }
      // CacheManager will ensure that directory tree path to saved file is built.
      if ( WowManager::Instance().cmgr.saveToCache(path, readBuf) < 0 ) {
        // failed to save to cache
        return -1;
      }
    } else {
      // there is no file on the server
      // open it to create, maybe just use Create? @TODO
      auto response = WowManager::Instance().client.Open(
        std::string(converted_path), fi->flags);
      if ( response.ret_ == -1 ) {
        LogWarn("Could not create file on server: " + std::string( converted_path ) );
        // we are not able to create file on the server
        return -response.server_errno_;
      }
    }

    ret = open(path, fi->flags);
    if ( ret == -1 ) {
      // and now we are inconsistent with the server, but this is largely benign
      // at this point there is a file on the server, but not on the client
      return -errno;     
    }
    
    // tell cache manager about this file 
    WowManager::Instance().cmgr.registerFileOpen( ret, path );

    fi->fh = ret;
    return 0;
}

int unreliable_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "read %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_READ);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    int fd;

    if (fi == NULL) {
	fd = open(path, O_RDONLY);
    } else {
	fd = fi->fh;
    }

    if (fd == -1) {
	return -errno;
    }

    ret = pread(fd, buf, size, offset);
    if (ret == -1) {
        ret = -errno;
    }

    if (fi == NULL) {
	close(fd);
    }

    return ret;
}

int unreliable_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "write %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_WRITE);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    int fd;
    (void) fi;
    if(fi == NULL) {
	    fd = open(path, O_WRONLY);
    } else {
	    fd = fi->fh;
    }

    if (fd == -1) {
	    return -errno;
    }

    ret = pwrite(fd, buf, size, offset);
    if (ret == -1) {
        ret = -errno;
    }


    if(fi == NULL) {
        close(fd);
    }

    return ret;
}

int unreliable_statfs(const char *path, struct statvfs *buf)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "statfs %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_STATFS);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = statvfs(path, buf);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_flush(const char *path, struct fuse_file_info *fi)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "flush %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_FLUSH);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = close(dup(fi->fh));
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_release(const char *path, struct fuse_file_info *fi)
{
    std::cerr << "release called " << std::endl;
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "release %s\n", path);
    fclose(file);
    
    int ret = error_inject(path, OP_RELEASE);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    if ( fi == nullptr || fi->fh == -1 ) {
      logline(std::string(path) + " release called on empty fileinfo");
      return -1; // should this call be considered a success
    }

    // write to server
    // validate file header
    if ( ! WowManager::Instance().cmgr.validate(path, fi->fh) ) {
      logline(std::string(path) + " failed to validate file header");
      return -1;
    }

    if ( ! WowManager::Instance().cmgr.isDirty(path, fi->fh) ) {
      logline(std::string(path) + " file is not dirty, nothing to write back");
      return 0;
    }

    // @TODO: user a better path conversion mechanism
    char converted_path[500];
    strcpy(converted_path, path);
    convert_path(converted_path);

    // read local buffer
    auto& readBuf = WowManager::Instance().cmgr.readFile(fi->fh);
    
    // we reach here, it means we need to writeback
    if ( ! readBuf.empty() ) {
      // we will only write if read was successful
      auto res = WowManager::Instance().client.Writeback(converted_path, readBuf);
      if ( res.ret_ == -1 ) {
        LogWarn("write back failed to server, local writes will be lost");
        return -res.server_errno_;
      }
    }

    auto fd = fi->fh;

    ret = close(fd);
    // tell CacheManager that the file has been closed
    WowManager::Instance().cmgr.registerFileClose( fd );

    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "fsync %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_FSYNC);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    if (datasync) {
#ifdef __APPLE__
        ret = fcntl(fi->fh, F_FULLFSYNC);
#else
        ret = fdatasync(fi->fh);
#endif
        if (ret == -1) {
            return -errno;
        }
    } else {
        ret = fsync(fi->fh);
        if (ret == -1) {
            return -errno;
        }
    }

    return 0;
}

#ifdef HAVE_XATTR
#ifdef __APPLE__
int unreliable_setxattr(const char *path, const char *name,
                        const char *value, size_t size, int flags, [[gnu::maybe_unused]]uint32_t pos)
#else
int unreliable_setxattr(const char *path, const char *name,
                        const char *value, size_t size, int flags)
#endif
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "setxattr %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_SETXATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

#ifdef __APPLE__
    ret = setxattr(path, name, value, size, 0, flags);
#else
    ret = setxattr(path, name, value, size, flags);
#endif /* __APPLE__ */
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

#ifdef __APPLE__
int unreliable_getxattr(const char *path, const char *name,
                        char *value, size_t size, [[gnu::maybe_unused]]uint32_t junk)
#else
int unreliable_getxattr(const char *path, const char *name,
                        char *value, size_t size)
#endif
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "getxattr %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_GETXATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    char converted_path[100];
    strcpy(converted_path, path);
    convert_path(converted_path);

    //Send request to server for file stat info.
    RPCResponse response = WowManager::Instance().client.GetXAttr(
        std::string(converted_path), std::string(name), value, size);

    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "\tgetxattr: response %d\n", response.ret_);

    //Verify response 
    if(response.ret_ == -1)
    {
        fprintf(file, "\tgetxattr: errno %d\n", response.server_errno_);
        fclose(file);
        return -response.server_errno_;
    }
    fclose(file);
    
    return 0;
}

int unreliable_listxattr(const char *path, char *list, size_t size)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "listxattr %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_LISTXATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

#ifdef __APPLE__
    ret = listxattr(path, list, size, XATTR_NOFOLLOW);
#else
    ret = listxattr(path, list, size);
#endif /* __APPLE__ */
    if (ret == -1) {
        return -errno;
    }
    
    return ret;
}

int unreliable_removexattr(const char *path, const char *name)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "removexattr %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_REMOVEXATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

#ifdef __APPLE__
    ret = removexattr(path, name, XATTR_NOFOLLOW);
#else
    ret = removexattr(path, name);
#endif /* __APPLE__ */
    if (ret == -1) {
        return -errno;
    }
    
    return 0;    
}
#endif /* HAVE_XATTR */

int unreliable_opendir(const char *path, struct fuse_file_info *fi)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "opendir %s\n", path);
    fclose(file);
    int ret = error_inject(path, OP_OPENDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    char converted_path[100];
    strcpy(converted_path, path);
    convert_path(converted_path);
    DIR *dir = opendir(converted_path);
    //DIR *dir = opendir(path);

    if (!dir) {
        return -errno;
    }
    fi->fh = (int64_t) dir;

    return 0;    
}

int unreliable_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "readdir %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_READDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    //DIR *dp = opendir(path);
    char converted_path[100];
    strcpy(converted_path, path);
    convert_path(converted_path);
    DIR *dp = opendir(converted_path);

    if (dp == NULL) {
	return -errno;
    }
    struct dirent *de;

    (void) offset;
    (void) fi;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }
    closedir(dp);

    return 0;
}

int unreliable_releasedir(const char *path, struct fuse_file_info *fi)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "releasedir %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_RELEASEDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    DIR *dir = (DIR *) fi->fh;

    ret = closedir(dir);

    if (ret == -1) {
        return -errno;
    }
    
    return 0;    
}

int unreliable_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "fsyncdir %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_FSYNCDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        return -errno;
    }

    if (datasync) {
 #ifdef __APPLE__
        ret = fcntl(dirfd(dir), F_FULLFSYNC);
 #else
        ret = fdatasync(dirfd(dir));
 #endif
        if (ret == -1) {
            return -errno;
        }
    } else {
        ret = fsync(dirfd(dir));
        if (ret == -1) {
            return -errno;
        }
    }
    closedir(dir);

    return 0;
}

void *unreliable_init(struct fuse_conn_info *conn)
{
    return NULL;
}

void unreliable_destroy(void *private_data)
{

}

int unreliable_access(const char *path, int mode)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "access %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_ACCESS);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    char converted_path[100];
    strcpy(converted_path, path);
    convert_path(converted_path);

    RPCResponse response = WowManager::Instance().client.Access(std::string(converted_path), mode);
    if (response.ret_ == -1) {
        fprintf(file, "access %s failed\n", path);
        return -response.server_errno_;
    }
    
    return 0;
}

int unreliable_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "create %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_CREAT);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    char converted_path[100];
    strcpy(converted_path, path);
    convert_path(converted_path);

    // Send request to server to create file
    RPCResponse response = WowManager::Instance().client.Create(
        std::string(converted_path), mode, fi->flags);

    file = fopen(WOWFS_LOG_FILE, "a");
    if (response.ret_ == -1) {
        fprintf(file, "\tserver create failed: errno %d\n", response.server_errno_);
        fclose(file);
        return -response.server_errno_;
    }
    fprintf(file, "\tserver create success\n");
    fclose(file);
    
    ret = open(path, fi->flags, mode);
    if ( ret == -1 ) {
      std::cerr << "create failed [ " << std::string(path) << " ] client failed" << std::endl;
      return -errno;
    }
    
    // tell CacheManager about this file
    WowManager::Instance().cmgr.registerFileOpen(ret, path);
    fi->fh = ret;
    return 0; 
}

int unreliable_ftruncate(const char *path, off_t length,
                         struct fuse_file_info *fi)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "ftruncate %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_FTRUNCATE);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = truncate(path, length);
    if (ret == -1) {
        return -errno;
    }
    
    return 0;    
}

int unreliable_fgetattr(const char *path, struct stat *buf,
                        struct fuse_file_info *fi)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "fgetattr %s\n", path);
    fclose(file);

    //If we want to retrieve the stat from remote (Option B)
    //Assuming we are saving local paths in the FHManager.
    //return unreliable_getattr(path, buf);

    //Option A
    //===========================
    //TODO: Once all functions are done, we may want to revisit this and make
    //sure it handles edge cases such as when fstat is called on a closed file handle

    int ret = error_inject(path, OP_FGETATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = fstat((int) fi->fh, buf);
    if (ret == -1) {
        return -errno;
    }
    
    return 0;    
    //===========================
}

int unreliable_lock(const char *path, struct fuse_file_info *fi, int cmd,
                    struct flock *fl)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "lock %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_LOCK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = fcntl((int) fi->fh, cmd, fl);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

#if !defined(__OpenBSD__)
int unreliable_ioctl(const char *path, int cmd, void *arg,
                     struct fuse_file_info *fi,
                     unsigned int flags, void *data)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "ioctl %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_IOCTL);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = ioctl(fi->fh, cmd, arg);
    if (ret == -1) {
        return -errno;
    }
    
    return ret;
}
#endif /* __OpenBSD__ */

#ifdef HAVE_FLOCK
int unreliable_flock(const char *path, struct fuse_file_info *fi, int op)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "flock %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_FLOCK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = flock(((int) fi->fh), op);
    if (ret == -1) {
        return -errno;
    }
    
    return 0;    
}
#endif /* HAVE_FLOCK */

#ifdef HAVE_FALLOCATE
int unreliable_fallocate(const char *path, int mode,
                         off_t offset, off_t len,
                         struct fuse_file_info *fi)
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "fallocate %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_FALLOCATE);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    int fd;
    (void) fi;

    if (mode) {
	return -EOPNOTSUPP;
    }

    if(fi == NULL) {
	fd = open(path, O_WRONLY);
    } else {
	fd = fi->fh;
    }

    if (fd == -1) {
	return -errno;
    }

    ret = fallocate((int) fi->fh, mode, offset, len);
    if (ret == -1) {
        return -errno;
    }

    if(fi == NULL) {
	close(fd);
    }
    
    return 0;    
}
#endif /* HAVE_FALLOCATE */

#ifdef HAVE_UTIMENSAT
int unreliable_utimens(const char *path, const struct timespec ts[2])
{
    FILE * file;
    file = fopen(WOWFS_LOG_FILE, "a");
    fprintf(file, "utimens %s\n", path);
    fclose(file);

    int ret = error_inject(path, OP_UTIMENS);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    /* don't use utime/utimes since they follow symlinks */
    ret = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}
#endif /* HAVE_UTIMENSAT */
