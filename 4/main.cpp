#include <iostream>
#include <filesystem>
#include <queue>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <string>
#include "ThreadPool.h"


namespace fs = std::filesystem;

struct [[maybe_unused]] HumanReadable {
    double size{};
private: friend
    std::ostream& operator<<(std::ostream& os, HumanReadable hr) {
        int i{};
        double mantissa = hr.size;
        for (; mantissa >= 1024.; mantissa /= 1024., ++i) { }
        mantissa = std::ceil(mantissa * 10.) / 10.;
        os << mantissa << "BKMGTPE"[i];
        return i == 0 ? os : os << "B (" << (uint64_t)hr.size << " B)";
    }
};


class Size {
private:
    fs::path task;
    double size;
public:
    explicit Size(std::string task): task(std::move(task)), size(0) {}
    void inc(uintmax_t s) { size += static_cast<double>(s);};
    ~Size () {
        std::cout << "> " << task << ": " <<  HumanReadable{size} << std::endl;
    }
};


void check_dir_threading(const std::string& start_dir, ThreadPool &pool, std::shared_ptr<Size> &size) {
    try {
        fs::directory_iterator begin(start_dir);
        fs::directory_iterator end;
        for (; begin != end; ++begin) {
            try {
                if (fs::is_directory(begin->path()) && !fs::is_symlink(begin->path())) {
                    pool.push(check_dir_threading, begin->path().string(), std::ref(pool), size);
                } else if (!fs::is_symlink(begin->path())){
                    size->inc(fs::file_size(begin->path()));
                }
            } catch (fs::filesystem_error &) {}
        }
    } catch(fs::filesystem_error &) {}
}


void start_task(const std::string& task, ThreadPool &pool) {
    pool.push(check_dir_threading, task, std::ref(pool), std::make_shared<Size>(task));
}


int main(int argc, char **argv) {
    int threads = 4;
    if ((argc == 3) && (!strcmp(argv[1], "-t") || !strcmp(argv[1], "--threads")))
        threads = std::max(std::stoi(argv[2]), 1);

    ThreadPool pool(threads);
    std::cout << "Using " << threads << " thread(s)!" << std::endl;

    std::string arg;
    while(true) {
        std::cin >> arg;
        if (arg == ":exit") {
            std::cout << "Awaiting incompleted tasks!" << std::endl;
            break;
        }
        if (arg == ":cancel") {
            std::cout << "Cancelling incompleted tasks!" << std::endl;
            pool.clear();
        }
        else {
            std::cout << "Starting task " << arg << "!" << std::endl;
            start_task(arg, pool);
        }
    }
    while (!pool.empty()) {}
    pool.close();
    return 0;
}