#pragma once

#include <string>
struct Servers
{
    std::string szHost = "80.242.59.112";
    int iPort = 7777;
};

inline Servers GetServers()
{
    Servers config;
    return config;
}