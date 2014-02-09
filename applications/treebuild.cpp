
#include "Freelist.h"
#include "WTPExceptions.h"
#include "WorkerThreadPool.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <assert.h>

using namespace std;
using namespace WTP;
WorkerThreadPool *wtp = NULL;
int globerr = 0;
int rndfd = 0;
Freelist freelist;

class DirProcessor : public WorkItem {
public:
    static void procdir(int dirfd, int level, int nsubdirs, int nfiles, uint64_t filesize);
    void setParams(int dirfd, int level, int nsubdirs, int nfiles,
                 uint64_t filesize) {
       _dirfd = dirfd;
       _level = level;
       _nsubdirs=nsubdirs;
      _nfiles=nfiles;
      _filesize=filesize;
    }
    void run() {
        procdir(_dirfd, _level, _nsubdirs, _nfiles, _filesize);
    }

    void reset() {
       _dirfd = -1;
       _level = 0;
       _nsubdirs=0;
      _nfiles=0;
      _filesize=0;
    }

private:
    int _dirfd, _level, _nsubdirs, _nfiles;
    uint64_t _filesize;
};

void DirProcessor::procdir(int dirfd, int level, int nsubdirs, int nfiles, uint64_t filesize)
{
    int fd = 0;
    int i = 0, rc = 0;
    ostringstream ost;

    if (globerr)
        return;
    // Create subdirs
    if (level > 0) {
       for (i=0; i< nsubdirs; i++) {
            ost << "dir" << i; 
            const char *dirname = ost.str().c_str();
            rc = mkdirat(dirfd, dirname, 0777); 
            if (rc < 0) {
                perror("mkdirat");
                throw string("mkdirat() failed");
            }
            fd = openat(dirfd, dirname, O_RDONLY);
            if (fd < 0) {
                perror("openat");
                throw string("openat() failed for subdir");
            }
            ost.clear(); ost.str("");
            /*
             * If a DP object is available from freelist, use that.
             * else do recursive processing.
             */
            DirProcessor *dp = dynamic_cast<DirProcessor *>(freelist.getItem());
            if (dp) {
                dp->setParams(fd, level-1, nsubdirs, nfiles, filesize);
                wtp->addWorkItem(dp);
            } else {
                procdir(fd, level-1, nsubdirs, nfiles, filesize);
            }
           
       }
    }

    char *buf = new char[filesize];

    for (i=0; i<nfiles; i++) {
        ost << "file" << i;
        const char * filename  = ost.str().c_str();
        int fd = openat(dirfd, filename,
                        O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd < 0) 
            throw string("openat() failed");
        if (read(rndfd, buf, filesize) != filesize)
            throw string("Failed to read from /dev/urandom");
        if (write(fd, buf, filesize) != filesize)
            throw string("Failed to write to file");
        close(fd);
        // cout << "YAAAA" << endl;
        ost.clear(); ost.str("");
    }

    delete[] buf;
    close(dirfd);

}

int main(int argc, char *argv[])
{
try {
    if (argc < 2)
        throw CallerError("usage: treebuild topdir [options ..]");
    rndfd = open("/dev/urandom", O_RDONLY);
    if (rndfd < 0) {
        perror("/dev/urandom");
        throw string("Failed to open /dev/urandom");
    }
    int dirfd = open(argv[1], O_RDONLY);
    if (dirfd < 0)
        throw string("Failed to open topdir.");

    int i=0;
    for (i=0; i< 1000; i++) {
        DirProcessor *dp = new DirProcessor();
        freelist.addItem(dp);
    }

    wtp = new WorkerThreadPool();

    DirProcessor *dp = new DirProcessor();
    dp->setParams(dirfd, 5, 4, 5, 1000);
    
    wtp->startThreads();

    wtp->addWorkItem(dp);

    cout << "Waiting for empty\n";
    wtp->waitEmpty();
    cout << "WTP empty. Waiting for shutdown.\n";
    wtp->shutDown();
    delete wtp;

    if (globerr < 0)
        cout << "Failure" << endl;

} catch (CallerError& exc) {
    cout << "Illegal use of WTP library: " << exc.getMessage() << endl;
}    

}
