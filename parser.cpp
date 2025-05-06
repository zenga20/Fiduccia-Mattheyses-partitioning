#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
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
        if (line.substr(0, 8) == "NumNets:") continue;
        if (line.substr(0, 8) == "NumPins:") continue;

        if (line.find("NetDegree") != string::npos) {
            istringstream iss(line);
            string dummy;
            int degree;
            iss >> dummy >> degree;
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


// Example main for parser testing
int main() {
    parseAux("/Users/dongyunlee/Documents/SBU_2025_Spring/ESE326/superblue18 2/superblue18.aux");
    
    return 0;
}
