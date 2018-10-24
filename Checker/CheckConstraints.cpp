#include <vector>
#include <iostream>

#include "CheckConstraints.h"

using namespace std;


CheckConstraints::CheckConstraints(const pb::PCenter::Input &input_s, const pb::PCenter::Output &output_s) {
    input = input_s;
    output = output_s;
    generateNum();
    generateGraph();
    generateMaxLength();
}

int CheckConstraints::generateNum() { // ��ȡ�ڵ���
    for (auto edge = input.graph().edges().begin(); edge != input.graph().edges().end(); ++edge) { // ���б��нڵ��ŵ����ֵ��Ϊ�ڵ���
        if (edge->source() > nodeNum) {
            nodeNum = edge->source();
        }
        if (edge->target() > nodeNum) {
            nodeNum = edge->target();
        }
    }
    return nodeNum;
}

vector<vector<int>> CheckConstraints::generateGraph() { 
    adjMartrix.resize(nodeNum, vector<int>(nodeNum, INF));
    for (auto edge = input.graph().edges().begin(); edge != input.graph().edges().end(); ++edge) { // �����������ٽӾ���
        adjMartrix[edge->source() - 1][edge->target() - 1] = edge->length();
        adjMartrix[edge->target() - 1][edge->source() - 1] = edge->length();
    }
    for (int i = 0; i < nodeNum; ++i) {
        adjMartrix[i][i] = 0;
    }
    floyd(adjMartrix); // ����floyd�㷨������ȫͼ���ٽӾ���
    return adjMartrix;
}

int CheckConstraints::generateMaxLength() {
    centerNum = input.centernum();
    vector<int> serverLengthList;
    for (int i = 0; i < nodeNum; ++i) {
        int serveLength = INF; // �ڵ�ķ���߳���
        for (int j = 0; j < centerNum; ++j) {
            int k = output.centers(j);
            if (serveLength > adjMartrix[i][k]) {
                serveLength = adjMartrix[i][k];
            }
        }
        serverLengthList.push_back(serveLength);
    }
    auto maxPosition_s = max_element(serverLengthList.begin(), serverLengthList.end());
    maxLength = *maxPosition_s; // ���з�����е����ֵ
    return maxLength;
}

void CheckConstraints::floyd(vector<vector<int>> &graph) {
    int nodeNum = (int)graph.size();
    for (int mid = 0; mid < nodeNum; ++mid) {
        for (int src = 0; src < nodeNum; ++src) {
            for (int dst = 0; dst < nodeNum; ++dst) {
                int distance = graph[src][mid] + graph[mid][dst];
                if (distance < graph[src][dst]) {
                    graph[src][dst] = distance;
                }
            }
        }
    }
}

