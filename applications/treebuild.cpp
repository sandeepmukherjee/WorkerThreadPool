
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

class DirProcessor : public WorkItem {
public:
    DirProcessor(int dirfd, int level) : _dirfd(dirfd), _level(level) {}
    void run() {
        int fd = 0;
        int i = 0, rc = 0;
        ostringstream ost;

        if (globerr)
            return;
        // Create subdirs
        if (_level > 0) {
           for (i=0; i< 4; i++) {
                ost << "dir" << i; 
                const char *dirname = ost.str().c_str();
                rc = mkdirat(_dirfd, dirname, 0777); 
                if (rc < 0) {
                    perror("mkdirat");
                    throw string("mkdirat() failed");
                }
                fd = openat(_dirfd, dirname, O_RDONLY);
                if (fd < 0) {
                    perror("openat");
                    throw string("openat() failed for subdir");
                }
                DirProcessor *dp = new DirProcessor(fd, _level-1);
                wtp->addWorkItem(dp);
                ost.clear(); ost.str("");
               
           }
        }

        char *buf = "YAAA";
        for (i=0; i<5; i++) {
            ost << "file" << i;
            const char * filename  = ost.str().c_str();
            int fd = openat(_dirfd, filename,
                            O_CREAT | O_WRONLY | O_TRUNC);
            if (fd < 0) 
                throw string("openat() failed");
            write(fd, buf, 6);
            close(fd);
            cout << "YAAAA" << endl;
            ost.clear(); ost.str("");
        }


        close(_dirfd);
            
    }

private:
    int _dirfd, _level;
};

int main(int argc, char *argv[])
{
try {
    if (argc < 2)
        throw CallerError("usage: treebuild topdir [options ..]");
    int dirfd = open(argv[1], O_RDONLY);
    if (dirfd < 0)
        throw CallerError("Failed to open topdir.");
    wtp = new WorkerThreadPool();

    DirProcessor *dp = new DirProcessor(dirfd, 3);
    
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
