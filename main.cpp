#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <queue>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cassert>

using namespace std;
struct Cell {
    string name;
    int width;
    int height;
    int x = 0;
    int y = 0;
    bool is_fixed = false;
};

struct FMCell {
    string name;
    int area;
    int partition; // 0 or 1
    bool locked = false;
    int gain = 0;
};

struct Net {
    string name;
    vector<string> pins;
};

struct Row {
    int coord_y;
    int height;
    int site_width;
    int site_spacing;
    int site_count;
    int origin_x;
};

const int MAX_GAIN = 100;  // Estimate max possible gain
const int BUCKET_SIZE = 2 * MAX_GAIN + 1;  // From -MAX_GAIN to +MAX_GAIN

vector<unordered_set<string>> gain_bucket_0(BUCKET_SIZE);
vector<unordered_set<string>> gain_bucket_1(BUCKET_SIZE);

unordered_map<string, Cell> cells;
vector<Net> nets;
vector<Row> rows;

void parseNodes(const string &filename) {
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#' || line.find("UCLA") != string::npos) continue;
        if (line.find("NumNodes") != string::npos) continue;
        if (line.find("NumTerminals") != string::npos) continue;

        istringstream iss(line);
        string name;
        int width, height;
        iss >> name >> width >> height;

        if (!name.empty()) {
            cells[name] = {name, width, height};
        }
    }
}


void parsePl(const string &filename) {
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        istringstream iss(line);
        string name;
        int x, y;
        string fixed;
        iss >> name >> x >> y;
        if (cells.find(name) != cells.end()) {
            cells[name].x = x;
            cells[name].y = y;
            if (line.find(":N") != string::npos) {
                cells[name].is_fixed = true;
            }
        }
    }
}

void parseNets(const string &filename) {
    ifstream file(filename);
    string line;
    int net_count = 0;

    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line.rfind("NumNets:", 0) == 0) continue;
        if (line.rfind("NumPins:", 0) == 0) continue;

        if (line.find("NetDegree") != string::npos) {
            istringstream iss(line);
            string dummy, colon;
            int degree;
            iss >> dummy >> colon >> degree;   // ✅ read the “:” into colon, then degree

            Net net;
            net.name = "net" + to_string(net_count++);
            for (int i = 0; i < degree; ++i) {
                getline(file, line);
                istringstream pin_line(line);
                string pin_name;
                pin_line >> pin_name;
                net.pins.push_back(pin_name);
            }
            nets.push_back(net);
        }
    }
}


void parseScl(const string &filename) {
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        if (line.find("CoreRow") != string::npos) {
            Row row;
            while (getline(file, line) && line.find("End") == string::npos) {
                if (line.find(":") == string::npos) continue;
                istringstream iss(line);
                string key;
                iss >> key;
                if (key == "Coordinate") {
                    string dummy;
                    iss >> dummy >> row.coord_y;
                } else if (key == "Height") {
                    string dummy;
                    iss >> dummy >> row.height;
                } else if (key == "Sitewidth") {
                    string dummy;
                    iss >> dummy >> row.site_width;
                } else if (key == "Sitespacing") {
                    string dummy;
                    iss >> dummy >> row.site_spacing;
                } else if (key == "NumSites") {
                    string dummy;
                    iss >> dummy >> row.site_count;
                } else if (key == "Origin") {
                    string dummy;
                    iss >> dummy >> row.origin_x;
                }
            }
            rows.push_back(row);
        }
    }
}

void parseAux(const string &filename) {
    ifstream file(filename);
    string line;
    getline(file, line);
    istringstream iss(line);

    string keyword;
    string colon;
    string nodesFile, netsFile, wtsFile, plFile, sclFile, shapesFile, routeFile;

    iss >> keyword >> colon >> nodesFile >> netsFile >> wtsFile >> plFile >> sclFile >> shapesFile >> routeFile;

    string basePath = "/Users/dongyunlee/Documents/SBU_2025_Spring/ESE326/superblue18 2/";

    cout << "nodes file: " << nodesFile << endl;
    cout << "nets file: " << netsFile << endl;
    cout << "pl file: " << plFile << endl;
    cout << "scl file: " << sclFile << endl;

    parseNodes(basePath + nodesFile);
    parseNets(basePath + netsFile);
    parsePl(basePath + plFile);
    parseScl(basePath + sclFile);

    cout << "Parsed " << cells.size() << " cells, " << nets.size() << " nets, " << rows.size() << " rows." << endl;
}



unordered_map<string, FMCell> fm_cells;
unordered_map<string, vector<string>> cell_to_nets;
unordered_map<string, Net> net_map;

// -------------------- FM Init --------------------
void initializeFM() {
    srand(time(0));

    // Initialize fm_cells from global "cells"
    for (const auto& [name, c] : cells) {
        if (c.is_fixed) continue;
        FMCell fc;
        fc.name = name;
        fc.area = c.width * c.height;
        fc.partition = rand() % 2;
        fm_cells[name] = fc;
    }

    for (const auto& net : nets) {
        Net new_net;
        new_net.name = net.name;
        for (const auto& pin : net.pins) {
            if (cells[pin].is_fixed) continue; // skip fixed pins completely
            new_net.pins.push_back(pin);
            cell_to_nets[pin].push_back(net.name);
        }
        net_map[net.name] = new_net;
    }
    int p0 = 0, p1 = 0;
    for (const auto& [name, c] : fm_cells) {
        if (c.partition == 0) p0++;
        else p1++;
    }
    cout << "Partition sizes: P0=" << p0 << " P1=" << p1 << endl;



}

// -------------------- Cut Size --------------------
int computeCutSize() {
    int cut = 0;
    for (const auto& [net_name, net] : net_map) {
        int part0 = 0, part1 = 0;
        for (const string& c : net.pins) {
            if (fm_cells.count(c) == 0) continue; // skip fixed cells
            if (fm_cells[c].partition == 0) part0++;
            else part1++;
        }
        if (part0 > 0 && part1 > 0) cut++;
    }
    return cut;
}

// -------------------- Gain Computation --------------------
void computeInitialGains() {
    for (auto& [name, cell] : fm_cells) {
        int gain = 0;
        for (const string& net_name : cell_to_nets[name]) {
            const auto& net = net_map[net_name];
            int from_part = 0;
            int to_part = 0;
            for (const string& c : net.pins) {
                if (c == name) continue;
                if (fm_cells[c].partition == cell.partition) from_part++;
                else to_part++;
            }
            if (from_part == 0) gain--;
            if (to_part == 0) gain++;
        }
        cell.gain = gain;
    }
}

void buildGainBuckets() {
    for (const auto& [name, cell] : fm_cells) {
        if (cell.locked) continue;
        int idx = cell.gain + MAX_GAIN;
        if (cell.partition == 0)
            gain_bucket_0[idx].insert(name);
        else
            gain_bucket_1[idx].insert(name);
    }
}

string getMaxGainCell(int partition) {
    auto& bucket = (partition == 0) ? gain_bucket_0 : gain_bucket_1;
    for (int i = BUCKET_SIZE - 1; i >= 0; --i) {
        if (!bucket[i].empty()) {
            return *bucket[i].begin();  // Return any one cell with max gain
        }
    }
    return "";  // No available cell
}

void removeFromBucket(const FMCell& cell) {
    int idx = cell.gain + MAX_GAIN;
    if (cell.partition == 0)
        gain_bucket_0[idx].erase(cell.name);
    else
        gain_bucket_1[idx].erase(cell.name);
}

void moveCell(const string& cell_name) {
    FMCell& cell = fm_cells[cell_name];

    removeFromBucket(cell);     // remove from old bucket
    cell.locked = true;         // lock it
    cell.partition = 1 - cell.partition;  // flip partition
}

void updateNeighborGains(const string& moved_cell) {
    int old_part = 1 - fm_cells[moved_cell].partition;

    for (const string& net_name : cell_to_nets[moved_cell]) {
        const auto& net = net_map[net_name];

        for (const string& neighbor : net.pins) {
            if (neighbor == moved_cell || fm_cells[neighbor].locked) continue;

            FMCell& c = fm_cells[neighbor];
            removeFromBucket(c);  // old gain bucket

            // Recompute gain
            int gain = 0;
            for (const string& net_name2 : cell_to_nets[neighbor]) {
                const auto& net2 = net_map[net_name2];
                int from_part = 0, to_part = 0;
                for (const string& cc : net2.pins) {
                    if (cc == neighbor) continue;
                    if (fm_cells[cc].partition == c.partition) from_part++;
                    else to_part++;
                }
                if (from_part == 0) gain--;
                if (to_part == 0) gain++;
            }
            c.gain = gain;

            // Re-insert with new gain
            int idx = c.gain + MAX_GAIN;
            if (c.partition == 0)
                gain_bucket_0[idx].insert(neighbor);
            else
                gain_bucket_1[idx].insert(neighbor);
        }
    }
}




// -------------------- Main --------------------
int main() {
    parseAux("superblue18.aux");
    initializeFM();
    computeInitialGains();

    int cut = computeCutSize();
    cout << "Initial cut size: " << cut << endl;

    // TODO: implement gain bucket, move loop, locking, best cut tracking

    buildGainBuckets();

    int best_cut = cut;
    unordered_map<string, int> best_partition;
    for (const auto& [name, c] : fm_cells)
        best_partition[name] = c.partition;

    for (int step = 0; step < fm_cells.size(); ++step) {
        int p0_size = count_if(fm_cells.begin(), fm_cells.end(), [](auto& p) {
            return p.second.partition == 0 && !p.second.locked;
        });
        int p1_size = count_if(fm_cells.begin(), fm_cells.end(), [](auto& p) {
            return p.second.partition == 1 && !p.second.locked;
        });

        // Balance: pick from larger side
        int move_from = (p0_size > p1_size) ? 0 : 1;
        string cell_to_move = getMaxGainCell(move_from);
        if (cell_to_move.empty()) break;

        moveCell(cell_to_move);
        updateNeighborGains(cell_to_move);

        cut = computeCutSize();
        cout << "Step " << step << ": moved " << cell_to_move << ", cut = " << cut << endl;

        if (cut < best_cut) {
            best_cut = cut;
            for (const auto& [name, c] : fm_cells)
                best_partition[name] = c.partition;
        }
    }
    for (auto& [name, c] : fm_cells)
        c.partition = best_partition[name];

    cout << "Best cut found: " << best_cut << endl;

    ofstream fout("partition_result.txt");
    for (const auto& [name, c] : fm_cells) {
        fout << name << " " << c.partition << "\n";
    }
    fout.close();

    return 0;
}
