#ifndef CHECKCONSTRAINTS_H
#define CHECKCONSTRAINTS_H

#include <vector>
#include <iostream>
#include"../Solver/PCenter.pb.h"


const int INF = 1000000000;


class CheckConstraints {
public:
    int nodeNum = 0;
    int centerNum = 0;
    int maxLength = 0;
    std::vector<std::vector<int>> adjMartrix;
    pb::PCenter::Input input;
    pb::PCenter::Output output;

public:
    CheckConstraints(const pb::PCenter::Input &input_s, const pb::PCenter::Output &output_s);

public:
    int generateNum();
    std::vector<std::vector<int>> generateGraph();
    int generateMaxLength();
    void floyd(std::vector<std::vector<int>> &graph);
};

#endif