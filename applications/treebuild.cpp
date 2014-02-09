
/**
 * THIS IS WORK IN PROGRESS!
 */

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
    static void randfile(int fd, uint64_t size);

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

void DirProcessor::randfile(int fd, uint64_t size)
{
    uint64_t remaining = size;
    char *buf = new char[4000];
    while (remaining > 0) {
        int toread = remaining > 4000 ? 4000 : remaining;
        int in = read(rndfd, buf, toread);
        if (in < 0)
            throw string("Failed to read /dev/urandom");
        int wrote = write(fd, buf, in);
        if (wrote < 0)
            throw string("Failed to write to file");
        remaining -= wrote;
    }

    delete[] buf;
}

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
            const string& dirname = ost.str();
            const char *dirname_c = dirname.c_str();
            rc = mkdirat(dirfd, dirname_c, 0777); 
            if (rc < 0) {
                perror("mkdirat");
                throw string("mkdirat() failed");
            }
            fd = openat(dirfd, dirname_c, O_RDONLY);
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


    for (i=0; i<nfiles; i++) {
        ost << "file" << i;
        const string& filename = ost.str();
        int fd = openat(dirfd, filename.c_str(),
                        O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd < 0) 
            throw string("openat() failed");
        randfile(fd, filesize);
        close(fd);
        ost.clear(); ost.str("");
    }

    close(dirfd);

}

int main(int argc, char *argv[])
{
    const int ndirs = 4;
    const int nfiles = 6;
    const int height = 6;
    const int filesize = 8000;

try {
    if (argc < 2)
        throw CallerError("usage: treebuild topdir [options ..]");
    rndfd = open("/dev/urandom", O_RDONLY);
    if (rndfd < 0) {
        perror("/dev/urandom");
        throw string("Failed to open /dev/urandom");
    }
    const char *topdir = argv[1];
    int rc = mkdir(topdir, 0777);
    if (rc < 0)
        throw string("Failed to create top level dir");
    int dirfd = open(topdir, O_RDONLY);
    if (dirfd < 0)
        throw string("Failed to open top level dir.");

    int i=0;
    for (i=0; i< 1000; i++) {
        DirProcessor *dp = new DirProcessor();
        freelist.addItem(dp);
    }

    wtp = new WorkerThreadPool(16);
    wtp->startThreads();

    DirProcessor *dp = new DirProcessor();
    dp->setParams(dirfd, height, ndirs, nfiles, filesize);

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
} catch(std::string& err) {
    cout << err << endl;
}

}
