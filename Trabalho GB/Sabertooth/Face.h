#pragma once
#include <vector>

class Face {
public:
    std::vector<int> v;   // índices de vértices
    std::vector<int> vt;  // índices de UV
    std::vector<int> vn;  // índices de normais
};