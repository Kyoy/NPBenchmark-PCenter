#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "Visualizer.h"

#include "../Solver/PbReader.h"
#include "../Solver/PCenter.pb.h"
#include "CheckConstraints.h"


using namespace std;
using namespace szx;
using namespace pb;



int main(int argc, char *argv[]) { 
    enum CheckerFlag {
        IoError = 0x0,
        FormatError = 0x1,
        CenterRepeatedError = 0x2,
    };

    /*string inputPath = "C:\\Users\\jinqi\\Desktop\\NPBenchmark-PCenter-master\\Deploy\\Instance\\pmed1.json";
    string outputPath = "C:\\Users\\jinqi\\Desktop\\NPBenchmark-PCenter-master\\Deploy\\Solution\\pmed1.json";*/

    string inputPath;
    string outputPath;

    if (argc > 1) {
        inputPath = argv[1];
    } else {
        cerr << "input path: " << flush;
        cin >> inputPath;
    }

    if (argc > 2) {
        outputPath = argv[2];
    } else {
        cerr << "output path: " << flush;
        cin >> outputPath;
    }

    pb::PCenter::Input input;
    if (!load(inputPath, input)) { return ~CheckerFlag::IoError; }

    pb::PCenter::Output output;
    ifstream ifs(outputPath);
    if (!ifs.is_open()) { return ~CheckerFlag::IoError; }

    string submission;
    getline(ifs, submission); // skip the first line.
    ostringstream oss;
    oss << ifs.rdbuf();
    jsonToProtobuf(oss.str(), output);

    // check solution.
    int error = 0;
    int maxLength = 0;
    if (output.centers().size() != input.centernum()) { error |= CheckerFlag::FormatError; }
    for (int i = 0; i < output.centers().size(); ++i) {
        for (int j = i + 1; j < output.centers().size(); ++j) {
            if (output.centers(i) == output.centers(j))
                error |= CheckerFlag::CenterRepeatedError;
        }
    }

    CheckConstraints checker(input, output);
    maxLength = checker.maxLength;

    int returnCode = (error == 0) ? maxLength: ~error;
    cout << returnCode << endl;
    return returnCode;
}
