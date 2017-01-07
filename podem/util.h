#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <vector>
#include <climits>
#include <string>
#include <algorithm>

using namespace std;
typedef std::string String;
class PatternForSort{
public:
    PatternForSort(String s, int i): pattern(s), detNum(i){}
    String pattern;
    int    detNum;
};
bool pat_comp(const PatternForSort &a, const PatternForSort &b)
    {return a.detNum > b.detNum;}

#endif
