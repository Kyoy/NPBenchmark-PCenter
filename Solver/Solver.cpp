#include "Solver.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>

#include <cmath>


using namespace std;

namespace szx {

#pragma region Solver::Cli
int Solver::Cli::run(int argc, char * argv[]) {
    Log(LogSwitch::Szx::Cli) << "parse command line arguments." << endl;
    Set<String> switchSet;
    Map<String, char*> optionMap({ // use string as key to compare string contents instead of pointers.
        { InstancePathOption(), nullptr },
        { SolutionPathOption(), nullptr },
        { RandSeedOption(), nullptr },
        { TimeoutOption(), nullptr },
        { MaxIterOption(), nullptr },
        { JobNumOption(), nullptr },
        { RunIdOption(), nullptr },
        { EnvironmentPathOption(), nullptr },
        { ConfigPathOption(), nullptr },
        { LogPathOption(), nullptr }
    });

    for (int i = 1; i < argc; ++i) { // skip executable name.
        auto mapIter = optionMap.find(argv[i]);
        if (mapIter != optionMap.end()) { // option argument.
            mapIter->second = argv[++i];
        } else { // switch argument.
            switchSet.insert(argv[i]);
        }
    }

    Log(LogSwitch::Szx::Cli) << "execute commands." << endl;
    if (switchSet.find(HelpSwitch()) != switchSet.end()) {
        cout << HelpInfo() << endl;
    }

    if (switchSet.find(AuthorNameSwitch()) != switchSet.end()) {
        cout << AuthorName() << endl;
    }

    Solver::Environment env;
    env.load(optionMap);
    if (env.instPath.empty() || env.slnPath.empty()) { return -1; }

    Solver::Configuration cfg;
    cfg.load(env.cfgPath);

    Log(LogSwitch::Szx::Input) << "load instance " << env.instPath << " (seed=" << env.randSeed << ")." << endl;
    Problem::Input input;
    if (!input.load(env.instPath)) { return -1; }

    Solver solver(input, env, cfg);
    solver.solve();

    pb::PCenter_Submission submission;
    submission.set_thread(to_string(env.jobNum));
    submission.set_instance(env.friendlyInstName());
    submission.set_duration(to_string(solver.timer.elapsedSeconds()) + "s");

    solver.output.save(env.slnPath, submission);
    #if JQ_DEBUG
    solver.output.save(env.solutionPathWithTime(), submission);
    solver.record();
    #endif // JQ_DEBUG

    return 0;
}
#pragma endregion Solver::Cli

#pragma region Solver::Environment
void Solver::Environment::load(const Map<String, char*> &optionMap) {
    char *str;

    str = optionMap.at(Cli::EnvironmentPathOption());
    if (str != nullptr) { loadWithoutCalibrate(str); }

    str = optionMap.at(Cli::InstancePathOption());
    if (str != nullptr) { instPath = str; }

    str = optionMap.at(Cli::SolutionPathOption());
    if (str != nullptr) { slnPath = str; }

    str = optionMap.at(Cli::RandSeedOption());
    if (str != nullptr) { randSeed = atoi(str); }

    str = optionMap.at(Cli::TimeoutOption());
    if (str != nullptr) { msTimeout = static_cast<Duration>(atof(str) * Timer::MillisecondsPerSecond); }

    str = optionMap.at(Cli::MaxIterOption());
    if (str != nullptr) { maxIter = atoi(str); }

    str = optionMap.at(Cli::JobNumOption());
    if (str != nullptr) { jobNum = atoi(str); }

    str = optionMap.at(Cli::RunIdOption());
    if (str != nullptr) { rid = str; }

    str = optionMap.at(Cli::ConfigPathOption());
    if (str != nullptr) { cfgPath = str; }

    str = optionMap.at(Cli::LogPathOption());
    if (str != nullptr) { logPath = str; }

    calibrate();
}

void Solver::Environment::load(const String &filePath) {
    loadWithoutCalibrate(filePath);
    calibrate();
}

void Solver::Environment::loadWithoutCalibrate(const String &filePath) {
    // EXTEND[szx][8]: load environment from file.
    // EXTEND[szx][8]: check file existence first.
}

void Solver::Environment::save(const String &filePath) const {
    // EXTEND[szx][8]: save environment to file.
}
void Solver::Environment::calibrate() {
    // adjust thread number.
    int threadNum = thread::hardware_concurrency();
    if ((jobNum <= 0) || (jobNum > threadNum)) { jobNum = threadNum; }

    // adjust timeout.
    msTimeout -= Environment::SaveSolutionTimeInMillisecond;
}
#pragma endregion Solver::Environment

#pragma region Solver::Configuration
void Solver::Configuration::load(const String &filePath) {
    // EXTEND[szx][5]: load configuration from file.
    // EXTEND[szx][8]: check file existence first.
}

void Solver::Configuration::save(const String &filePath) const {
    // EXTEND[szx][5]: save configuration to file.
}
#pragma endregion Solver::Configuration

#pragma region Solver
bool Solver::solve() {
    init();

    int workerNum = (max)(1, env.jobNum / cfg.threadNumPerWorker);
    cfg.threadNumPerWorker = env.jobNum / workerNum;
    List<Solution> solutions(workerNum, Solution(this));
    List<bool> success(workerNum);

    Log(LogSwitch::Szx::Framework) << "launch " << workerNum << " workers." << endl;
    List<thread> threadList;
    threadList.reserve(workerNum);
    for (int i = 0; i < workerNum; ++i) {
        // TODO[szx][2]: as *this is captured by ref, the solver should support concurrency itself, i.e., data members should be read-only or independent for each worker.
        // OPTIMIZE[szx][3]: add a list to specify a series of algorithm to be used by each threads in sequence.
        threadList.emplace_back([&, i]() { success[i] = optimize(solutions[i], i); });
    }
    for (int i = 0; i < workerNum; ++i) { threadList.at(i).join(); }

    Log(LogSwitch::Szx::Framework) << "collect best result among all workers." << endl;
    int bestIndex = -1;
    Length bestValue = 100000;
    for (int i = 0; i < workerNum; ++i) {
        if (!success[i]) { continue; }
        Log(LogSwitch::Szx::Framework) << "worker " << i << " got " << solutions[i].maxLength << endl;
        //cout << solutions[i].centers().size();
        if (solutions[i].maxLength >= bestValue) { continue; }
        bestIndex = i;
        bestValue = solutions[i].maxLength;
    }
    env.rid = to_string(bestIndex);
    if (bestIndex < 0) { return false; }
    output = solutions[bestIndex];
    return true;
}

void Solver::record() const {
    #if JQ_DEBUG
    int generation = 0;

    ostringstream log;

    System::MemoryUsage mu = System::peakMemoryUsage();

    Length obj = output.maxLength;
    //cout << obj << endl;
    Length checkerObj = -1;
    bool feasible = check(checkerObj);

    // record basic information.
    log << env.friendlyLocalTime() << ","
        << env.rid << ","
        << env.instPath << ","
        << feasible << "," << (obj - checkerObj) << ","
        << output.maxLength << ","
        << timer.elapsedSeconds() << ","
        << mu.physicalMemory << "," << mu.virtualMemory << ","
        << env.randSeed << ","
        << cfg.toBriefStr() << ","
        << generation << "," << iteration << ",";
        

    // record solution vector.
    // EXTEND[szx][2]: save solution in log.
    log << endl;

    // append all text atomically.
    static mutex logFileMutex;
    lock_guard<mutex> logFileGuard(logFileMutex);

    ofstream logFile(env.logPath, ios::app);
    logFile.seekp(0, ios::end);
    if (logFile.tellp() <= 0) {
        logFile << "Time,ID,Instance,Feasible,ObjMatch,Width,Duration,PhysMem,VirtMem,RandSeed,Config,Generation,Iteration" << endl;
    }
    logFile << log.str();
    logFile.close();
    #endif // JQ_DEBUG
}

bool Solver::check(Length &checkerObj) const {
    #if JQ_DEBUG
    enum CheckerFlag {
        IoError = 0x0,
        FormatError = 0x1,
        CenterRepeatedError = 0x2,
    };

    checkerObj = System::exec("Checker.exe " + env.instPath + " " + env.solutionPathWithTime());
    if (checkerObj > 0) { return true; }
    checkerObj = ~checkerObj;
    if (checkerObj == CheckerFlag::IoError) { Log(LogSwitch::Checker) << "IoError." << endl; }
    if (checkerObj & CheckerFlag::FormatError) { Log(LogSwitch::Checker) << "FormatError." << endl; }
    if (checkerObj & CheckerFlag::CenterRepeatedError) { Log(LogSwitch::Checker) << "CenterRepeatedError." << endl; }
    return false;
    #else
    checkerObj = 0;
    return true;
    #endif // JQ_DEBUG
}

void Solver::init() {
    int nodeNum = 0;
    for (auto edge = input.graph().edges().begin(); edge != input.graph().edges().end(); ++edge) { // 所有边中节点编号的最大值即为节点数
        if (edge->source() > nodeNum) {
            nodeNum = edge->source();
        }
        if (edge->target() > nodeNum) {
            nodeNum = edge->target();
        }
    }
    aux.nodeNum = nodeNum; 
    aux.edgeNum = input.graph().edges().size();
    aux.centerNum = input.centernum();

    aux.adjMartrix.resize(aux.nodeNum, List<int>(aux.nodeNum, INF));
    for (auto edge = input.graph().edges().begin(); edge != input.graph().edges().end(); ++edge) { // 生成算例的临接矩阵
        aux.adjMartrix[edge->source() - 1][edge->target() - 1] = edge->length();
        aux.adjMartrix[edge->target() - 1][edge->source() - 1] = edge->length();
    }
    for (int i = 0; i < aux.nodeNum; ++i) {
        aux.adjMartrix[i][i] = 0;
    }
    Floyd::floyd(aux.adjMartrix); // 利用floyd算法生成完全图的临接矩阵
}

bool Solver::optimize(Solution &sln, ID workerId) {
    Log(LogSwitch::Szx::Framework) << "worker " << workerId << " starts." << endl;
    bool status = true;
    sln.maxLength = 0;

    // TODO[0]: replace the following random assignment with your own algorithm.
    for (int e = 0; !timer.isTimeOut() && (e < aux.centerNum); ++e) { // 随机生成服务节点p
        int index = rand.pick(0, aux.nodeNum);
        sln.add_centers(index);
    }

    vector<int> serverLengthList; // 存储所有服务边
    serverLengthList.reserve(aux.nodeNum);
    for (int i = 0; i < aux.nodeNum; ++i) {
        int serveLength = INF; // 节点的服务边长度
        for (int j = 0; j < aux.centerNum; ++j) {
            int k = sln.centers(j);
            if (serveLength > aux.adjMartrix[i][k]) {
                serveLength = aux.adjMartrix[i][k];
            }
        }
        serverLengthList.push_back(serveLength); 
    }
    auto maxPosition_s = max_element(serverLengthList.begin(), serverLengthList.end());
    sln.maxLength = *maxPosition_s; // 所有服务边中的最大值，即问题的输出


    Log(LogSwitch::Szx::Framework) << "worker " << workerId << " ends." << endl;
    return status;
}
#pragma endregion Solver

}
