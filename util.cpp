#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <signal.h> // for kill()
#include <sys/syscall.h>
#include <sys/stat.h>
#include <execinfo.h> // for backtrace()
#include <cxxabi.h>   // for abi::__cxa_demangle()
#include <algorithm>  // for std::transform()
#include <sstream>
#include "util.h"

namespace MyServer
{
    pid_t GetThreadId()
    {
        return syscall(SYS_gettid);
    }

    uint64_t GetFiberId()
    {
        return Fiber::GetFiberId();
    }

    uint64_t GetElapsedMS()
    {
        struct timespec ts = {0};
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }

    std::string GetThreadName()
    {
        char *thread_name = nullptr;
        pthread_getname_np(pthread_self(), thread_name, 16);
        return std::string(thread_name);
    }

    void SetThreadName(const std::string &name)
    {
        pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
    }

    static std::string demangle(const char *str)
    {
        std::string rt;
        rt.resize(256);
        if (1 == sscanf(str, "%*[^(]%*[^_]%255[^)+]", &rt[0]))
        {
            char *v = abi::__cxa_demangle(&rt[0], nullptr, nullptr, nullptr);
            if (v)
            {
                std::string result(v);
                free(v);
                return result;
            }
        }
        if (1 == sscanf(str, "%255s", &rt[0]))
        {
            return rt;
        }
        return str;
    }

    void Backtrace(std::vector<std::string> &bt, int size, int skip)
    {
        void **array = (void **)malloc(sizeof(void *) * size);
        size_t s = backtrace(array, size);
        char **strings = backtrace_symbols(array, s);
        if (strings == NULL)
        {
            std::cout << "backtrace_synbols error";
            return;
        }
        for (size_t i = skip; i < s; i++)
        {
            bt.push_back(demangle(strings[i]));
        }
        free(strings);
        free(array);
    }

    std::string BacktraceToString(int size, int skip, const std::string &prefix)
    {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); i++)
        {
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }

    uint64_t GetCurrentMS()
    {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
    }

    uint64_t GetCurrentUS()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
    }

    std::string ToLower(const std::string &str)
    {
        std::string ret = str;
        std::transform(ret.begin(), ret.end(), ret.begin(), tolower);
        return ret;
    }

    std::string ToUpper(const std::string &str)
    {
        std::string ret = str;
        std::transform(ret.begin(), ret.end(), ret.begin(), toupper);
        return ret;
    }

    std::string Time2Str(time_t ts, const std::string &format)
    {
        struct tm tm;
        localtime_r(&ts, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), format.c_str(), &tm);
        return std::string(buf);
    }

    time_t Str2Time(const char *str, const char *format)
    {
        struct tm tm;
        memset(&tm, 0, sizeof(tm));
        if (!strptime(str, format, &tm))
            return 0;
        return mktime(&tm);
    }

    void FSUtil::ListAllFile(std::vector<std::string> &files, const std::string &path, const std::string &subfix)
    {
        if (access(path.c_str(), 0) != 0)
            return;
        DIR *dir = opendir(path.c_str());
        if (dir == nullptr)
            return;
        struct dirent *dp = nullptr;
        while ((dp = readdir(dir)) != nullptr)
        {
            if (dp->d_type == DT_DIR)
            {
                if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
                    continue;
                ListAllFile(files, path + "/" + dp->d_name, subfix);
            }
            else if (dp->d_type == DT_REG)
            {
                std::string filename(dp->d_name);
                if (subfix.empty())
                {
                    files.push_back(path + "/" + filename);
                }
                else
                {
                    if (filename.size() > subfix.size() && filename.substr(filename.length() - subfix.size()) == subfix)
                    {
                        files.push_back(path + "/" + filename);
                    }
                }
            }
        }
        closedir(dir);
    }

    static int __lstat(const char *file, struct stat *st = nullptr) //存在返回0
    {
        struct stat lst;
        int ret = lstat(file, &lst);
        if (st)
        {
            *st = lst;
        }
        return ret;
    }

    static int __mkdir(const char *dirname)
    {
        if (access(dirname, F_OK) == 0)
            return 0;
        return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    bool FSUtil::Mkdir(const std::string &dirname)
    {
        if(__lstat(dirname.c_str())==0) return true;
        char* path = strdup(dirname.c_str());
        char* ptr = strchr(path+1,'/');
        do
        {
            for(;ptr;*ptr='/',ptr=strchr(ptr+1,'/'))
            {
                *ptr = '\0';
                if(__mkdir(path)!=0) break;
            }
            if(ptr!=nullptr) break;
            if(__mkdir(path)!=0) break;
            free(path);
            return true;
        } while (0);
        free(path);
        return false;
    }

    bool FSUtil::IsRunningPidfile(const std::string &pidfile)
    {
        if(__lstat(pidfile.c_str())!=0) return false;
        std::ifstream ifs(pidfile);
        std::string line;
        if(!ifs || !std::getline(ifs,line) || line.empty())
        {
            return false;
        }
        pid_t pid = atoi(line.c_str());
        if(pid<=1) return false;
        if(kill(pid,0)!=0) return false;
        return true;
    }

    bool FSUtil::Unlink(const std::string &filename,bool exist)
    {
        if(!exist && __lstat(filename.c_str())) return true;
        return unlink(filename.c_str())==0;
    }

    bool FSUtil::Rm(const std::string &path)
    {
        struct stat st;
        if(lstat(path.c_str(),&st)) return true;
        if(!(st.st_mode & S_IFDIR)) return Unlink(path);
        DIR* dir = opendir(path.c_str());
        if(!dir) return false;
        struct dirent *dp = nullptr;
        bool ret = true;
        while(dp=readdir(dir))
        {
            if(!strcmp(dp->d_name,".")||!strcmp(dp->d_name,"..")) continue;
            std::string dirname = path + "/" + dp->d_name;
            ret = Rm(dirname);
        }
        closedir(dir);
        if(rmdir(path.c_str())) ret = false;
        return ret;
    }

    bool FSUtil::Mv(const std::string& from,const std::string& to)
    {
        if(!Rm(to)) return false;
        return rename(from.c_str(),to.c_str());
    }

    bool FSUtil::Realpath(const std::string &path,std::string &rpath)
    {
        if(__lstat(path.c_str())) return false;
        char* ptr = realpath(path.c_str(),nullptr);
        if(ptr==nullptr) return false;
        std::string(ptr).swap(rpath);
        free(ptr);
        return true;
    }

    bool FSUtil::Symlink(const std::string &from,const std::string &to)
    {
        if(!Rm(to)) return false;
        return symlink(from.c_str(),to.c_str()) == 0;
    }

    std::string FSUtil::Dirname(const std::string &filename)
    {
        if(filename.empty()) return ".";
        auto pos = filename.rfind('/');
        if(pos==0) return "/";
        else if(pos == std::string::npos) return ".";
        else return filename.substr(0,pos);
    }

    std::string FSUtil::Basename(const std::string &filename)
    {
        if(filename.empty()) return filename;
        auto pos = filename.rfind('/');
        if(pos==std::string::npos) return filename;
        else return filename.substr(pos+1);
    }

    bool FSUtil::OpenForRead(std::ifstream &ifs,const std::string &filename,std::ios_base::openmode mode)
    {
        ifs.open(filename.c_str(),mode);
        return ifs.is_open();
    }

    bool FSUtil::OpenForWrite(std::ofstream &ofs, const std::string &filename, std::ios_base::openmode mode)
    {
        ofs.open(filename.c_str(),mode);
        if(!ofs.is_open())
        {
            std::string dir = Dirname(filename);
            Mkdir(dir);
            ofs.open(filename.c_str(),mode);
        }
        return ofs.is_open();
    }

    

} // end myserver