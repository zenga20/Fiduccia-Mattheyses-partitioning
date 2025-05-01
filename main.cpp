#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <cstdlib>

using namespace std;
// -------------------- Data Structures --------------------
struct Cell {
    string name;
    int area;
    int partition; // 0 or 1
    bool locked = false;
    int gain = 0;
};

struct Net {
    string name;
    vector<string> cells;
};

unordered_map<string, Cell> cells;
vector<Net> nets;

// -------------------- Parsers --------------------
void parse_nodes(const string &filename) {
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        istringstream iss(line);
        string name;
        int area = 1; // default area
        iss >> name;
        if (line.find(":") != string::npos) {
            string dummy;
            iss >> dummy >> area;
        }
        cells[name] = {name, area, -1};
    }
}

void parse_nets(const string &filename) {
    ifstream file(filename);
    string line;
    Net current;
    while (getline(file, line)) {
        if (line.find("NetDegree") != string::npos) {
            if (!current.name.empty()) {
                nets.push_back(current);
                current = Net();
            }
            current.name = "net" + to_string(nets.size());
        } else {
            string cell;
            istringstream iss(line);
            iss >> cell;
            current.cells.push_back(cell);
        }
    }
    if (!current.name.empty()) {
        nets.push_back(current);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <.nodes file> <.nets file>\n";
        return 1;
    }

    parse_nodes(argv[1]);
    parse_nets(argv[2]);

    cout << "Parsed " << cells.size() << " cells and " << nets.size() << " nets.\n";

    // TODO: implement initial partition
    // TODO: implement FM algorithm loop
    // TODO: track cut size and area balance

    return 0;
}
