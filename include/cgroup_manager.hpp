#ifndef CGROUP_MANAGER_HPP
#define CGROUP_MANAGER_HPP

#include <string>
#include <memory>
#include <iostream>

class CGroupMetrics {
public:
    long long cpu_usage_ns;
    long long memory_usage_bytes;
    long long memory_limit_bytes;
    std::string cgroup_version;
    CGroupMetrics();
    void print() const;
};

class CGroupManager {
private:
    std::string detectCGroupVersion() const;
    std::string readFile(const std::string& path) const;
    long long parseLong(const std::string& str) const;
    bool fileExists(const std::string& path) const;

public:
    CGroupManager();
    std::unique_ptr<CGroupMetrics> getSystemMetrics() const;
};

#endif

