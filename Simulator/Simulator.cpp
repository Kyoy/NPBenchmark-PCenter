#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <random>

#include <cstring>

#include "Simulator.h"
#include "ThreadPool.h"


using namespace std;


namespace szx {

void Simulator::initDefaultEnvironment() {
    Solver::Environment env;
    env.save(Env::DefaultEnvPath());

    Solver::Configuration cfg;
    cfg.save(Env::DefaultCfgPath());
}

void Simulator::run(const Task &task) {
    String instanceName(task.instSet + task.instId + ".json");
    String solutionName(task.instSet + task.instId + ".json");

    char argBuf[Cmd::MaxArgNum][Cmd::MaxArgLen];
    char *argv[Cmd::MaxArgNum];
    for (int i = 0; i < Cmd::MaxArgNum; ++i) { argv[i] = argBuf[i]; }
    strcpy(argv[ArgIndex::ExeName], ProgramName().c_str());

    int argc = ArgIndex::ArgStart;

    strcpy(argv[argc++], Cmd::InstancePathOption().c_str());
    strcpy(argv[argc++], (InstanceDir() + instanceName).c_str());

    System::makeSureDirExist(SolutionDir());
    strcpy(argv[argc++], Cmd::SolutionPathOption().c_str());
    strcpy(argv[argc++], (SolutionDir() + solutionName).c_str());

    if (!task.randSeed.empty()) {
        strcpy(argv[argc++], Cmd::RandSeedOption().c_str());
        strcpy(argv[argc++], task.randSeed.c_str());
    }

    if (!task.timeout.empty()) {
        strcpy(argv[argc++], Cmd::TimeoutOption().c_str());
        strcpy(argv[argc++], task.timeout.c_str());
    }

    if (!task.maxIter.empty()) {
        strcpy(argv[argc++], Cmd::MaxIterOption().c_str());
        strcpy(argv[argc++], task.maxIter.c_str());
    }

    if (!task.jobNum.empty()) {
        strcpy(argv[argc++], Cmd::JobNumOption().c_str());
        strcpy(argv[argc++], task.jobNum.c_str());
    }

    if (!task.runId.empty()) {
        strcpy(argv[argc++], Cmd::RunIdOption().c_str());
        strcpy(argv[argc++], task.runId.c_str());
    }

    if (!task.cfgPath.empty()) {
        strcpy(argv[argc++], Cmd::ConfigPathOption().c_str());
        strcpy(argv[argc++], task.cfgPath.c_str());
    }

    if (!task.logPath.empty()) {
        strcpy(argv[argc++], Cmd::LogPathOption().c_str());
        strcpy(argv[argc++], task.logPath.c_str());
    }

    Cmd::run(argc, argv);
}

void Simulator::run(const String &envPath) {
    char argBuf[Cmd::MaxArgNum][Cmd::MaxArgLen];
    char *argv[Cmd::MaxArgNum];
    for (int i = 0; i < Cmd::MaxArgNum; ++i) { argv[i] = argBuf[i]; }
    strcpy(argv[ArgIndex::ExeName], ProgramName().c_str());

    int argc = ArgIndex::ArgStart;

    strcpy(argv[argc++], Cmd::EnvironmentPathOption().c_str());
    strcpy(argv[argc++], envPath.c_str());

    Cmd::run(argc, argv);
}

void Simulator::debug() {
    Task task;
    task.instSet = "";
    task.instId = "pmed3";
    //task.randSeed = "1500972793";
    task.randSeed = to_string(Random::generateSeed());
    task.timeout = "180";
    //task.maxIter = "1000000000";
    task.jobNum = "1";
    task.cfgPath = Env::DefaultCfgPath();
    task.logPath = Env::DefaultLogPath();
    task.runId = "0";

    run(task);
}

void Simulator::benchmark(int repeat) {
    Task task;
    task.instSet = "";
    //task.timeout = "180";
    //task.maxIter = "1000000000";
    task.timeout = "3600";
    //task.maxIter = "1000000000";
    task.jobNum = "1";
    task.cfgPath = Env::DefaultCfgPath();
    task.logPath = Env::DefaultLogPath();

    random_device rd;
    mt19937 rgen(rd());
    // EXTEND[szx][5]: read it from InstanceList.txt.
    vector<String> instList({ "pmed1", "pmed2", "pmed3", "pmed4", "pmed5", "pmed6", "pmed7", "pmed8", "pmed9", "pmed10",
        "pmed11", "pmed12", "pmed13", "pmed14", "pmed15", "pmed16", "pmed17", "pmed18", "pmed19", "pmed20",
        "pmed21", "pmed22", "pmed23", "pmed24", "pmed25", "pmed26", "pmed27", "pmed28", "pmed29", "pmed30",
        "pmed31", "pmed32", "pmed33", "pmed34", "pmed35", "pmed36", "pmed37", "pmed38", "pmed39", "pmed40", });
    for (int i = 0; i < repeat; ++i) {
        //shuffle(instList.begin(), instList.end(), rgen);
        for (auto inst = instList.begin(); inst != instList.end(); ++inst) {
            task.instId = *inst;
            task.randSeed = to_string(Random::generateSeed());
            task.runId = to_string(i);
            run(task);
        }
    }
}

void Simulator::parallelBenchmark(int repeat) { 
    Task task;
    task.instSet = "";
    //task.timeout = "180";
    //task.maxIter = "1000000000";
    task.timeout = "3600";
    //task.maxIter = "1000000000";
    task.jobNum = "1";
    task.cfgPath = Env::DefaultCfgPath();
    task.logPath = Env::DefaultLogPath();

    ThreadPool<> tp(4);

    random_device rd;
    mt19937 rgen(rd());
    // EXTEND[szx][5]: read it from InstanceList.txt.
    vector<String> instList({ "pmed1", "pmed2", "pmed3", "pmed4", "pmed5", "pmed6", "pmed7", "pmed8", "pmed9", "pmed10",
        "pmed11", "pmed12", "pmed13", "pmed14", "pmed15", "pmed16", "pmed17", "pmed18", "pmed19", "pmed20",
        "pmed21", "pmed22", "pmed23", "pmed24", "pmed25", "pmed26", "pmed27", "pmed28", "pmed29", "pmed30",
        "pmed31", "pmed32", "pmed33", "pmed34", "pmed35", "pmed36", "pmed37", "pmed38", "pmed39", "pmed40", });
    for (int i = 0; i < repeat; ++i) {
        //shuffle(instList.begin(), instList.end(), rgen);
        for (auto inst = instList.begin(); inst != instList.end(); ++inst) {
            task.instId = *inst;
            task.randSeed = to_string(Random::generateSeed());
            task.runId = to_string(i);
            tp.push([=]() { run(task); });
            this_thread::sleep_for(1s);
        }
    }
}

void Simulator::generateInstance(const string location, const int num) {
    Problem::Input input;
    string str;
    int edgeNum;
    vector<int> line;
    //location = "D:\\vs-c\\IOstream\\instance\\pmed1.txt";
    ifstream infile(location);
    if (!infile.is_open()) { 
        cerr << "can not open file!";
        return;
    }
    getline(infile, str);
    line = splitString(str, " ");
    input.set_centernum(line[2]);
    edgeNum = line[1];
    while (!infile.eof()) {
        getline(infile, str);
        line = splitString(str, " ");
        auto &edge(*input.mutable_graph()->add_edges());
        edge.set_source(line[0]);
        edge.set_target(line[1]);
        edge.set_length(line[2]);
    }
    ostringstream path;
    path << InstanceDir() << "pmed"<< num << ".json";
    save(path.str(), input);
    //cout << "done" << endl;
}

vector<int> Simulator::splitString(const string& str, const char* separator) {
    vector<int> charVector;
    char *input = (char *)str.c_str();
    char *next_token = nullptr;
    char *p = strtok_s(input, separator, &next_token);
    int a;
    while (p != NULL) {
        sscanf_s(p, "%d", &a);
        charVector.push_back(a);
        p = strtok_s(NULL, separator, &next_token);
    }
    return charVector;
}

}

