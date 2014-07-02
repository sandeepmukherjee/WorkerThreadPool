
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
#include <unistd.h>
#include <getopt.h>
#include <map>

using namespace std;
using namespace WTP;
WorkerThreadPool *wtp = NULL;
int globerr = 0;
int rndfd = 0; // Filehandle to /dev/urandom. Not set if cpulite is in effect
Freelist freelist;
int cpulite = 0;
char *litebuf = NULL;

class DirProcessor : public WorkItem {
public:
    static void procdir(int dirfd, int level, int nsubdirs, int nfiles, uint64_t filesize);
    static void randfile(int fd, uint64_t size);
    static void randfile_lite(int fd, uint64_t size);

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

void DirProcessor::randfile_lite(int fd, uint64_t size)
{
    uint64_t remaining = size;
    while (remaining > 0) {
        int towrite = remaining > 4000 ? 4000 : remaining;
        int wrote = write(fd, litebuf, towrite);
        if (wrote < 0)
            throw string("Failed to write to file");
        remaining -= wrote;
    }

}
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
        if (cpulite)
            randfile_lite(fd, filesize);
        else
            randfile(fd, filesize);
        close(fd);
        ost.clear(); ost.str("");
    }

    close(dirfd);

}

int usage_exit(const string& progname, const string& err)
{
    cerr << "Error: " << string(err) << endl;
    cerr << "Usage: " << progname << 
           " --height=height --nfiles=files-perdir --height=tree-height" <<
           " --ndirs=subdirs-per-dir --filesize=file-size [--cpulite] topdir" << 
            endl;

    exit(-1);

}

static std::map<string, int *> valmap;
void setGlobalVal(const string& progname, const string& optname, const string& optvalstr)
{
    std::map<string, int *>::iterator iter = valmap.find(optname);
    assert (iter != valmap.end());
    int *valptr = valmap[optname];
    *valptr = atoi(optvalstr.c_str());
    if (*valptr <= 0)
        usage_exit(progname, string("Option argument must be positive integer"));
}

int main(int argc, char *argv[])
{

    int ndirs = 4;
    int nfiles = 6;
    int height = 6;
    int filesize = 8000;
    const string progname(argv[0]);

    valmap[string("ndirs")] = &ndirs;
    valmap[string("nfiles")] = &nfiles;
    valmap[string("height")] = &height;
    valmap[string("filesize")] = &filesize;
    valmap[string("cpulite")] = &cpulite;

    int option_index = 0;
    const char *topdir = NULL;
    struct option longopts[] = {
    {"ndirs", required_argument,  0, 0},
    {"nfiles", required_argument,  0, 0},
    {"height", required_argument,  0, 0},
    {"filesize", required_argument,  0, 0},
    {"cpulite", no_argument,  NULL, 1},
    {0, 0, 0, 0}
    };

    while (1) {
        int c = getopt_long(argc, argv, "", longopts, &option_index);
        if (c == -1)
            break;

        string optname(longopts[option_index].name);
        if (c == 0) {
            string optvalstr(optarg);
            setGlobalVal(progname, optname, optvalstr);
        } else {
            setGlobalVal(progname, optname, string("1"));
        }
    }

    if (optind == (argc-1)) {
       topdir = argv[optind];
    } else {
        usage_exit(progname, string("Insufficient arguments"));
    }

try {
    rndfd = open("/dev/urandom", O_RDONLY);
    if (rndfd < 0) {
        perror("/dev/urandom");
        throw string("Failed to open /dev/urandom");
    }
    if (cpulite) {
	    litebuf = new char[4000];
        read(rndfd, litebuf, 4000);
        close(rndfd);
	}
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
    if (cpulite)
        delete[] litebuf;

    if (globerr < 0)
        cout << "Failure" << endl;

} catch (InternalError& exc) {
    cout << "WTP internal bug or system is in unstable state: " << exc.getMessage() << endl;
} catch (CallerError& exc) {
    cout << "Illegal use of WTP library: " << exc.getMessage() << endl;
} catch(std::string& err) {
    cout << err << endl;
}

}
