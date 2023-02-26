#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

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

#include "WowLocalWriteReorder.H"
#include "unreliablefs_ops.h"
#include "WowLogger.H"
#include <fstream>
#include <string>
#include <vector>

#include <random>
#include <algorithm>


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

int unreliable_lstat(const char *path, struct stat *buf)
{
    LogInfo("lstat: " + std::string(path));

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
    LogInfo("getattr: " + std::string(path));

    int ret = error_inject(path, OP_GETATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }
    
    std::string converted_path = WowManager::Instance().removeMountPrefix(path);

    memset(buf, 0, sizeof(struct stat));

    auto statMeta = WowManager::Instance().shouldFetch( path );

    // if we can't find any stat, then we just error out
    if ( statMeta.source == WowManager::NONE ) {
      return -statMeta.errorCode; // this is server's error code
    } else {
      *buf = statMeta.statData;
      return 0;
    }
}

int unreliable_readlink(const char *path, char *buf, size_t bufsiz)
{
    LogInfo("readlink: " + std::string(path));

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
    LogInfo("mknod: " + std::string(path));

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
    LogInfo("mkdir: " + std::string(path));

    int ret = error_inject(path, OP_MKDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    std::string converted_path = WowManager::Instance().removeMountPrefix(path);

    // Send request to server to create directory.
    RPCResponse response = WowManager::Instance().client.Mkdir(
        converted_path, mode);

    if (response.ret_ == -1) {
        LogWarn(std::string("server mkdir failed: errno=") + std::to_string(response.server_errno_));
        return -response.server_errno_;
    }

    // Create local directory.
    ret = mkdir(path, mode);
    if (ret == -1) {
        LogWarn(std::string("local mkdir failed: errno=") + std::to_string(errno));
        return -errno;
    }

    return 0;
}

int unreliable_unlink(const char *path)
{
    LogInfo("unlink: " + std::string(path));

    int ret = error_inject(path, OP_UNLINK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    // unlink at server
    std::string converted_path = WowManager::Instance().removeMountPrefix(path);

    RPCResponse response = WowManager::Instance().client.Unlink(converted_path);

    if (response.ret_ == -1) {
      LogWarn("server unlink: failed errno=" + std::to_string(response.server_errno_));
      return -response.server_errno_;
    }

    // Unlink locally
    WowManager::Instance().cmgr.deleteFromCache(std::string(path));

    return 0;
}

int unreliable_rmdir(const char *path)
{
    LogInfo("rmdir: " + std::string(path));

    int ret = error_inject(path, OP_RMDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    std::string converted_path = WowManager::Instance().removeMountPrefix(path);

    // Send request to server to remove directory.
    RPCResponse response = WowManager::Instance().client.Rmdir(converted_path);

    if (response.ret_ == -1) {
        LogWarn("server rmdir failed: errno=" + std::to_string(response.server_errno_));
        return -response.server_errno_;
    }

    // Remove local directory.
    ret = rmdir(path);
    if (ret == -1) {
        LogWarn("local rmdir failed: errno=" + std::to_string(errno));
        return -1;
    }

    return 0;
}

int unreliable_symlink(const char *target, const char *linkpath)
{
    LogInfo("symlink: " + std::string(target) + " -> " + std::string(linkpath));

    int ret = error_inject(target, OP_SYMLINK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }
    std::string converted_target = WowManager::Instance().removeMountPrefix(target);

    std::string converted_linkpath = WowManager::Instance().removeMountPrefix(linkpath);

    RPCResponse response = WowManager::Instance().client.Symlink(
        converted_target, converted_linkpath);
    
    if ( response.ret_ == -1 ) {
      LogWarn("remote symlink failed for target=" + std::string(target) +  " linkpath=" + 
              std::string(linkpath) + " errno=" + std::to_string(response.server_errno_) );
      // We couldn't create a symlink on the server, but we are not going to crash just
      // yet.
    }


    ret = symlink(target, linkpath);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_rename(const char *oldpath, const char *newpath)
{
    LogInfo("rename: " + std::string(oldpath) + " -> " + std::string(newpath));

    int ret = error_inject(oldpath, OP_RENAME);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    // rename at server
    std::string converted_oldpath = WowManager::Instance().removeMountPrefix(oldpath);

    std::string converted_newpath = WowManager::Instance().removeMountPrefix(newpath);

    RPCResponse response = WowManager::Instance().client.Rename(
        std::string(converted_oldpath), std::string(converted_newpath));

    if (response.ret_ == -1) {
      LogWarn("rename: failed errno=" + std::to_string(response.server_errno_));  
      return -response.server_errno_;
    }

    // rename locally
    WowManager::Instance().cmgr.rename(std::string(oldpath), std::string(newpath));

    return 0;
}

int unreliable_link(const char *oldpath, const char *newpath)
{
    LogInfo("link: " + std::string(oldpath) + " -> " + std::string(newpath));

    int ret = error_inject(oldpath, OP_LINK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    std::string converted_oldpath = WowManager::Instance().removeMountPrefix(oldpath);

    std::string converted_newpath = WowManager::Instance().removeMountPrefix(newpath);

    RPCResponse response = WowManager::Instance().client.Link(
        converted_oldpath, converted_newpath);

    if (response.ret_ == -1) {
      LogWarn("link: failed errno=" + std::to_string(response.server_errno_));  
      return -response.server_errno_;
    }

    // link locally
    ret = link(oldpath, newpath);
    if (ret < 0) {
        return -errno;
    }

    return 0;
}

int unreliable_chmod(const char *path, mode_t mode)
{
    LogInfo("chmod: " + std::string(path));

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
    LogInfo("chown: " + std::string(path));

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
    LogInfo("truncate: " + std::string(path));

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
    LogInfo("open: " + std::string(path));
 
    int ret = error_inject(path, OP_OPEN);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    std::string converted_path = WowManager::Instance().removeMountPrefix(path);

    // check if the file is already available in cache
    fi->fh = 0;

    auto statMeta = WowManager::Instance().shouldFetch( path );

    // @TODO: Random Thought: if the file is not present on the server, we shouldn't
    // create it (just yet). It should be created on close. But this snowballs into
    // a big change, so not making it for now.
    // this means there is no stat on server, we should create a file on the server
    if ( ! statMeta.remotePresent ) {
      // there is no file on the server
      // open it to create, maybe just use Create? @TODO
      auto response = WowManager::Instance().client.Open(
        converted_path, fi->flags);
      if ( response.ret_ == -1 ) {
        LogWarn("Could not create file on server: " + converted_path);
        // we are not able to create file on the server
        return -response.server_errno_;
      }      
    }

    // source of most recent stat is server, so we should fetch
    if ( statMeta.source == WowManager::REMOTE ) {
      auto serverStat = statMeta.statData;
      // there is a file on the server, let's pull it
      auto fileSize = serverStat.st_size;
      std::string readBuf;
      auto response = WowManager::Instance().client.DownloadFile(
        converted_path, readBuf, fileSize);
      if ( response.ret_ == -1 ) {
        // there is a file on the server, but we are not able to read it!
        return -response.server_errno_;
      }
      // CacheManager will ensure that directory tree path to saved file is built.
      if ( ! WowManager::Instance().cmgr.saveToCache(path, readBuf) ) {
        // failed to save to cache, @TODO: should we let it fall through?
      }
    }
  
    WowManager::Instance().cmgr.constructDirPath( path );
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
    LogInfo("read: " + std::string(path));

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

//Execute write through WowFS
int wow_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
    int fd, ret;
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
    } else {
      // @FIXME: there is this other path here, when fi is NULL
      // not handling it for now
      WowManager::Instance().cmgr.registerFileDirty( fd );
    }

    return ret;
}

int unreliable_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
    LogInfo("write: " + std::string(path));

    int ret = error_inject(path, OP_WRITE);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    WowLocalWriteReorder::Instance().RecordWrite(path, buf, size, offset, fi);

    //Let application think we performed the write
    return size;
    //return wow_write(path, buf, size, offset, fi);
}

int unreliable_statfs(const char *path, struct statvfs *buf)
{
    LogInfo("statfs: " + std::string(path));

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
    LogInfo("flush: " + std::string(path));

    int ret = error_inject(path, OP_FLUSH);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    //Shuffle pending writes for out-of-order property
    WowLocalWriteReorder::Instance().Shuffle();

    for(auto op : *(WowLocalWriteReorder::Instance().GetQueue()))
    {
        LogWarn("Writing : " + op.buf);
        if(op.buf == WOW_KILL_PHRASE)
        {
            LogError("Kill phrase detected, goodbye...");
            raise(SIGSEGV);
        }
        wow_write(op.path.c_str(), op.buf.c_str(), op.size, op.offset, &op.fi);
    }

    // Note: When a file is closed the _release(...) is called async'ly
    // This violates the guarantee one generally associates with close.
    // This writeback here ensures that data goes to the server, provided
    // a user calls flush before close.
    if ( fi->fh > 0 ) {
      WowManager::Instance().writebackToServer( path, fi->fh );
    }
    
    ret = close(dup(fi->fh));
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_release(const char *path, struct fuse_file_info *fi)
{
    LogInfo("release: " + std::string(path));
    
    int ret = error_inject(path, OP_RELEASE);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    if ( fi == nullptr || fi->fh <= 0 ) {
      LogWarn(std::string(path) + " release called on empty fileinfo");
      return -1; // should this call be considered a success
    }

    // write to server
    // validate file header
    if ( ! WowManager::Instance().cmgr.validate(path, fi->fh) ) {
      LogWarn(std::string(path) + " failed to validate file header");
      return -1;
    }

    // dirty-file checks happen within the writeback method
    if ( ! WowManager::Instance().writebackToServer( path, fi->fh ) ) {
      return -1; // if we have reached here, this means the writeback has failed
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
    LogInfo("fsync: " + std::string(path));

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
    LogInfo("setxattr: " + std::string(path));

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
    LogInfo("getxattr: " + std::string(path));

    int ret = error_inject(path, OP_GETXATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    // if the source of most recent stat is server, we will fetch xattr from the server too
    if ( WowManager::Instance().shouldFetch( path ).source == WowManager::REMOTE ) {
        //Send request to server for file stat info.
        std::string fsPath = WowManager::Instance().removeMountPrefix(path);
        RPCResponse response = WowManager::Instance().client.GetXAttr(
            fsPath, std::string(name), value, size);

        //Verify response 
        if ( response.ret_ == -1) {
            LogWarn("getxattr failed: errno=" + std::to_string(response.server_errno_));
            return -response.server_errno_;
        }
    } else {
        #ifdef __APPLE__
            ret = getxattr(path, name, value, size, 0, XATTR_NOFOLLOW);
        #else
            ret = getxattr(path, name, value, size);
        #endif /* __APPLE__ */
            if ( ret < 0 ) {
                LogWarn("getxattr local failed: errno=" + std::to_string(errno));
                return -errno;
            }
    }
    return 0;
}

int unreliable_listxattr(const char *path, char *list, size_t size)
{
    LogInfo("listxattr: " + std::string(path));

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
    LogInfo("removexattr: " + std::string(path));

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
    LogWarn("unreliable_opendir called, and we don't know what to do with it: " + std::string(path));
    return 0;

    /* @FIXME
    int ret = error_inject(path, OP_OPENDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    std::string converted_path = WowManager::Instance().removeMountPrefix(path);

    std::string dir_buf;
    auto response = WowManager::Instance().client.DownloadDir(converted_path, dir_buf);
    if (response.ret_ < 0)
    {
        return -errno;
    }
    */
    
    //TODO: Temp fix for "cat ./subdir/otherfile" crash
    //WowManager::Instance().cmgr.saveToCache(path, dir_buf);
    //fi->fh = (int64_t)fopen(path, "rb");

    return 0;    
}


int unreliable_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
    LogInfo("readdir: " + std::string(path));

    int ret = error_inject(path, OP_READDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    //DIR *dp = opendir(path);
    std::string converted_path = WowManager::Instance().removeMountPrefix(path);
    
    std::string dir_buf;
    auto response = WowManager::Instance().client.DownloadDir(converted_path, dir_buf);
    if (response.ret_ < 0)
    {
        return -errno;
    }
    
    const char * data_ptr = dir_buf.c_str();
    struct dirent *de;
    for(unsigned int i = 0; i < dir_buf.size(); i+=sizeof(struct dirent))
    {
        de = (struct dirent*)(data_ptr+i);

        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    return 0;
}

int unreliable_releasedir(const char *path, struct fuse_file_info *fi)
{
    LogWarn("unreliable_releasedir called, and we don't know what to do with it: " + std::string(path));
    return 0;

    /* @FIXME
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
    */
    
    return 0;    
}

int unreliable_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
    LogInfo("fsyncdir: " + std::string(path));

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
    LogInfo("access: " + std::string(path));

    int ret = error_inject(path, OP_ACCESS);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    std::string converted_path = WowManager::Instance().removeMountPrefix(path);

    RPCResponse response = WowManager::Instance().client.Access(converted_path, mode);
    if (response.ret_ == -1) {
        LogWarn("access: failed for " + std::string(path) + " errno="
          + std::to_string(response.server_errno_));
        return -response.server_errno_;
    }

    return 0;
}

int unreliable_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi)
{
    LogInfo("create: " + std::string(path));

    int ret = error_inject(path, OP_CREAT);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    std::string converted_path = WowManager::Instance().removeMountPrefix(path);

    // TODO: Random Thought: we shouldn't create the file on the server just yet
    // Send request to server to create file
    
    RPCResponse response = WowManager::Instance().client.Create(
        converted_path, mode, fi->flags);

    if (response.ret_ == -1) {
        LogWarn("server create failed: errno=" + std::to_string(response.server_errno_));
        return -response.server_errno_;
    }
    
    WowManager::Instance().cmgr.constructDirPath( path ); 
    ret = open(path, fi->flags, S_IRWXU | S_IRWXG | S_IRWXO);
    
    if ( ret == -1 ) {
      LogWarn("client create failed: errno " + std::to_string(errno));
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
    LogInfo("ftruncate: " + std::string(path));

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
    
    // @FIXME: in general a user can call ftruncate with the length set
    // to the file size - this will result in dirty in the current impl.
    if ( fi->fh > 0 ) {
      WowManager::Instance().cmgr.registerFileDirty( fi->fh );
    }
   
    return 0;    
}

int unreliable_fgetattr(const char *path, struct stat *buf,
                        struct fuse_file_info *fi)
{
    LogInfo("fgetattr: " + std::string(path));

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
    LogInfo("lock: " + std::string(path));

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
    LogInfo("ioctl: " + std::string(path));

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
    LogInfo("flock: " + std::string(path));

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
    LogInfo("fallocate: " + std::string(path));

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
    // @FIXME: if offset + len != filesize then we should be marking this
    // file as dirty. For now we assume it as always dirty.
    if(fi == NULL) {
	close(fd);
    } else {
      WowManager::Instance().cmgr.registerFileDirty( fi-> fh );
    }
    
    return 0;    
}
#endif /* HAVE_FALLOCATE */

#ifdef HAVE_UTIMENSAT
int unreliable_utimens(const char *path, const struct timespec ts[2])
{
    LogInfo("utimens: " + std::string(path));

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
