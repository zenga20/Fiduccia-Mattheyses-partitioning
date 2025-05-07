#include <unordered_map>
#include <unordered_set>
#include <set>
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
#include <filesystem>
#include <chrono>

using namespace std;

// Area definition enum
enum class AreaDefinition {
    GATE_COUNT,
    GATE_AREA
};

// Global area definition
AreaDefinition current_area_definition = AreaDefinition::GATE_AREA;

struct Cell {
    string name;
    int width;
    int height;
    int x = 0;
    int y = 0;
    bool is_fixed = false;
};

struct FMCell {
    string name;  // Keep name for output reference
    int id = -1;  // New field: integer identifier
    int area = 0;
    int partition = 0; // 0 or 1
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

// Track max gain indices for each partition
int max_gain_0 = -MAX_GAIN;
int max_gain_1 = -MAX_GAIN;

// Use vector<int> for gain buckets instead of unordered_set<string>
vector<vector<int>> gain_bucket_0(BUCKET_SIZE);
vector<vector<int>> gain_bucket_1(BUCKET_SIZE);

// Track which buckets are non-empty
vector<bool> non_empty_0(BUCKET_SIZE, false);
vector<bool> non_empty_1(BUCKET_SIZE, false);

unordered_map<string, Cell> cells;
vector<Net> nets;
vector<Row> rows;

// Integer indexing data structures
unordered_map<string, int> cell_name_to_id; // Maps cell names to integer IDs
vector<FMCell> fm_cells_vec;                // Vector of FM cells indexed by IDs
vector<vector<int>> cell_to_nets_vec;       // Vector of nets for each cell by ID
vector<vector<int>> net_to_cells_vec;       // Vector of cells for each net by ID
unordered_map<string, int> net_name_to_id;  // Maps net names to integer IDs
unordered_map<int, string> net_id_to_name;  // Maps net IDs back to names

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
            iss >> dummy >> colon >> degree;   // ✅ read the ":" into colon, then degree

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
    cout << "Attempting to open .aux file: " << filename << endl;
    string basePath= filesystem::current_path().parent_path().string() + "/superblue18 2/";
    cout << "Looking for aux files in this directory: " << basePath << endl;
    string fullPath= basePath + filename;
    cout << "Full path to aux file: " << fullPath << endl;
    ifstream file(fullPath);
    if (!file.is_open()) {
        throw runtime_error("Error: Could not open .aux file: " + fullPath);
    }
    cout << "Opened .aux file: " << filename << endl;
    string line;
    getline(file, line);
    cout << "Read line from .aux file: " << line << endl;
    istringstream iss(line);

    string keyword;
    string colon;
    string nodesFile, netsFile, wtsFile, plFile, sclFile, shapesFile, routeFile;

    iss >> keyword >> colon >> nodesFile >> netsFile >> wtsFile >> plFile >> sclFile >> shapesFile >> routeFile;

    

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



// -------------------- FM Init --------------------
void initializeFM() {
    srand(time(0));

    // First assign integer IDs to cells and nets for faster indexing
    int cell_id = 0;
    int net_id = 0;
    
    // Create cell ID mappings
    for (const auto& [name, c] : cells) {
        if (c.is_fixed) continue;
        cell_name_to_id[name] = cell_id++;
    }
    
    // Initialize FM cells vector and cell-to-nets vector
    fm_cells_vec.resize(cell_id);
    cell_to_nets_vec.resize(cell_id);
    
    // Create net ID mappings and net-to-cells vector
    for (const auto& net : nets) {
        net_name_to_id[net.name] = net_id;
        net_id_to_name[net_id] = net.name;
        net_id++;
    }
    net_to_cells_vec.resize(net_id);
    
    // Initialize fm_cells with area and random partitions
    for (const auto& [name, c] : cells) {
        if (c.is_fixed) continue;
        int id = cell_name_to_id[name];
        
        FMCell& fc = fm_cells_vec[id];
        fc.name = name;
        fc.id = id;
        
        // Set area based on selected definition
        if (current_area_definition == AreaDefinition::GATE_COUNT) {
            fc.area = 1;  // Each gate counts as 1
        } else {
            fc.area = c.width * c.height;  // Use actual gate area
        }
        fc.partition = rand() % 2;
    }
    
    // Build net-to-cells and cell-to-nets relationships
    for (const auto& net : nets) {
        int net_id = net_name_to_id[net.name];
        for (const auto& pin : net.pins) {
            if (cells.find(pin) == cells.end() || cells[pin].is_fixed) continue;
            if (cell_name_to_id.find(pin) == cell_name_to_id.end()) continue; // Skip pins not found in mapping
            int cell_id = cell_name_to_id[pin];
            net_to_cells_vec[net_id].push_back(cell_id);
            cell_to_nets_vec[cell_id].push_back(net_id);
        }
    }
    
    int p0 = 0, p1 = 0;
    for (const auto& cell : fm_cells_vec) {
        if (cell.partition == 0) p0++;
        else p1++;
    }
    cout << "Partition sizes: P0=" << p0 << " P1=" << p1 << endl;
}

// -------------------- Cut Size --------------------
int computeCutSize() {
    int cut = 0;
    for (int net_id = 0; net_id < net_to_cells_vec.size(); net_id++) {
        int part0 = 0, part1 = 0;
        for (int cell_id : net_to_cells_vec[net_id]) {
            if (fm_cells_vec[cell_id].partition == 0) part0++;
            else part1++;
        }
        if (part0 > 0 && part1 > 0) cut++;
    }
    return cut;
}

// -------------------- Gain Computation --------------------
void computeInitialGains() {
    for (int cell_id = 0; cell_id < fm_cells_vec.size(); cell_id++) {
        int gain = 0;
        for (int net_id : cell_to_nets_vec[cell_id]) {
            int from_part = 0;
            int to_part = 0;
            for (int neighbor_id : net_to_cells_vec[net_id]) {
                if (neighbor_id == cell_id) continue;
                if (fm_cells_vec[neighbor_id].partition == fm_cells_vec[cell_id].partition) 
                    from_part++;
                else 
                    to_part++;
            }
            if (from_part == 0) gain++;
            if (to_part == 0) gain--;
        }
        fm_cells_vec[cell_id].gain = gain;
    }
}

void buildGainBuckets() {
    // Reset tracking arrays
    fill(non_empty_0.begin(), non_empty_0.end(), false);
    fill(non_empty_1.begin(), non_empty_1.end(), false);
    
    // Clear all buckets
    for (auto& bucket : gain_bucket_0) bucket.clear();
    for (auto& bucket : gain_bucket_1) bucket.clear();
    
    max_gain_0 = -MAX_GAIN;
    max_gain_1 = -MAX_GAIN;
    
    for (int cell_id = 0; cell_id < fm_cells_vec.size(); cell_id++) {
        const auto& cell = fm_cells_vec[cell_id];
        if (cell.locked) continue;
        
        int idx = cell.gain + MAX_GAIN;
        if (idx < 0) idx = 0;
        if (idx >= BUCKET_SIZE) idx = BUCKET_SIZE - 1;
        
        if (cell.partition == 0) {
            gain_bucket_0[idx].push_back(cell_id);
            non_empty_0[idx] = true;
            max_gain_0 = max(max_gain_0, cell.gain);
        } else {
            gain_bucket_1[idx].push_back(cell_id);
            non_empty_1[idx] = true;
            max_gain_1 = max(max_gain_1, cell.gain);
        }
    }
}

int getMaxGainCell(int partition) {
    auto& bucket = (partition == 0) ? gain_bucket_0 : gain_bucket_1;
    auto& non_empty = (partition == 0) ? non_empty_0 : non_empty_1;
    int& max_gain = (partition == 0) ? max_gain_0 : max_gain_1;
    
    while (max_gain >= -MAX_GAIN) {
        int idx = max_gain + MAX_GAIN;
        if (non_empty[idx] && !bucket[idx].empty()) {
            return bucket[idx][0]; // Return first cell ID with max gain
        }
        max_gain--;
    }
    return -1; // No available cell found
}

void removeFromBucket(int cell_id) {
    const auto& cell = fm_cells_vec[cell_id];
    int idx = cell.gain + MAX_GAIN;
    if (idx < 0) idx = 0;
    if (idx >= BUCKET_SIZE) idx = BUCKET_SIZE - 1;
    
    auto& bucket = (cell.partition == 0) ? gain_bucket_0[idx] : gain_bucket_1[idx];
    
    // Find and remove the cell from the bucket
    auto it = find(bucket.begin(), bucket.end(), cell_id);
    if (it != bucket.end()) {
        // Swap with the last element and pop (faster than erase)
        *it = bucket.back();
        bucket.pop_back();
    }
    
    // Update non-empty status
    if (bucket.empty()) {
        if (cell.partition == 0) non_empty_0[idx] = false;
        else non_empty_1[idx] = false;
    }
}

void moveCell(int cell_id) {
    FMCell& cell = fm_cells_vec[cell_id];
    removeFromBucket(cell_id);
    cell.locked = true;
    cell.partition = 1 - cell.partition;
}

void updateNeighborGains(int moved_cell_id) {
    unordered_set<int> affected_cells;  // Track cells that need gain updates
    
    for (int net_id : cell_to_nets_vec[moved_cell_id]) {
        for (int neighbor_id : net_to_cells_vec[net_id]) {
            if (neighbor_id == moved_cell_id || fm_cells_vec[neighbor_id].locked) continue;
            affected_cells.insert(neighbor_id);
        }
    }

    // Batch process gain updates
    for (int neighbor_id : affected_cells) {
        FMCell& c = fm_cells_vec[neighbor_id];
        removeFromBucket(neighbor_id);

        // Recompute gain
        int gain = 0;
        for (int net_id : cell_to_nets_vec[neighbor_id]) {
            int from_part = 0, to_part = 0;
            for (int cell_id : net_to_cells_vec[net_id]) {
                if (cell_id == neighbor_id) continue;
                if (fm_cells_vec[cell_id].partition == c.partition) from_part++;
                else to_part++;
            }
            if (from_part == 0) gain++;
            if (to_part == 0) gain--;
        }
        c.gain = gain;

        // Re-insert with new gain
        int idx = c.gain + MAX_GAIN;
        if (idx < 0) idx = 0;
        if (idx >= BUCKET_SIZE) idx = BUCKET_SIZE - 1;
        
        if (c.partition == 0) {
            gain_bucket_0[idx].push_back(neighbor_id);
            non_empty_0[idx] = true;
            max_gain_0 = max(max_gain_0, c.gain);
        } else {
            gain_bucket_1[idx].push_back(neighbor_id);
            non_empty_1[idx] = true;
            max_gain_1 = max(max_gain_1, c.gain);
        }
    }
}

int incrementalUpdateCut(int moved_cell_id, int current_cut) {
    const auto& cell = fm_cells_vec[moved_cell_id];
    int old_partition = 1 - cell.partition; // The partition before the move
    int delta = 0; // Change in cut size

    for (int net_id : cell_to_nets_vec[moved_cell_id]) {
        // Count pins in each partition before the move
        int old_part0 = 0, old_part1 = 0;
        for (int cell_id : net_to_cells_vec[net_id]) {
            if (cell_id == moved_cell_id) {
                // Use the old partition for the moved cell
                if (old_partition == 0) old_part0++;
                else old_part1++;
            } else {
                // Use current partition for other cells
                if (fm_cells_vec[cell_id].partition == 0) old_part0++;
                else old_part1++;
            }
        }
        
        // Count pins in each partition after the move
        int new_part0 = 0, new_part1 = 0;
        for (int cell_id : net_to_cells_vec[net_id]) {
            if (fm_cells_vec[cell_id].partition == 0) new_part0++;
            else new_part1++;
        }
        
        // Check if the net's cut status changed
        bool was_cut = (old_part0 > 0 && old_part1 > 0);
        bool is_cut = (new_part0 > 0 && new_part1 > 0);
        
        if (was_cut && !is_cut) delta--; // Net is no longer cut
        if (!was_cut && is_cut) delta++; // Net is now cut
    }
    
    return current_cut + delta;
}

// -------------------- Main --------------------
int main() {
    
    
    // Prompt user for area definition
    cout << "Choose area definition for FM algorithm:" << endl;
    cout << "1. Number of gates (each gate counts as 1)" << endl;
    cout << "2. Area of gates (width * height)" << endl;
    cout << "Enter choice (1 or 2): ";
    
    int choice;
    while (true) {
        cin >> choice;
        if (choice == 1) {
            current_area_definition = AreaDefinition::GATE_COUNT;
            cout << "Using gate count as area definition" << endl;
            break;
        } else if (choice == 2) {
            current_area_definition = AreaDefinition::GATE_AREA;
            cout << "Using gate area as area definition" << endl;
            break;
        } else {
            cout << "Invalid choice. Please enter 1 or 2: ";
        }
    }
    auto start = chrono::high_resolution_clock::now();

    try {
        cout << "Current working directory: " << filesystem::current_path() << endl;
        parseAux("superblue18.aux");
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return 1;
    }
    initializeFM();
    computeInitialGains();

    int cut = computeCutSize();
    cout << "Initial cut size: " << cut << endl;

    buildGainBuckets();

    int best_cut = cut;
    vector<int> best_partition(fm_cells_vec.size());
    for (int i = 0; i < fm_cells_vec.size(); i++) {
        best_partition[i] = fm_cells_vec[i].partition;
    }

    // Add multiple passes
    for (int pass = 0; pass < 10; pass++) {
        // Unlock all cells
        for (auto& cell : fm_cells_vec) {
            cell.locked = false;
        }
        
        // Rebuild buckets
        buildGainBuckets();
        
        int pass_best_cut = computeCutSize();
        int pass_best_step = 0;
        vector<int> moved_cells;
        vector<int> cut_history;
        int no_improvement_count = 0;
        const int max_no_improvement = 50; // Increased for better optimization
        
        cut_history.push_back(pass_best_cut);
        
        // Move cells until all are locked or no more improvement
        for (int step = 0; step < fm_cells_vec.size(); step++) {
            // Cache p0/p1 areas for better performance
            static int p0_area = 0, p1_area = 0;
            
            // Only recalculate areas every 100 steps or at beginning
            if (step == 0) {
                p0_area = 0;
                p1_area = 0;
                for (const auto& cell : fm_cells_vec) {
                    if (!cell.locked) {
                        if (cell.partition == 0) p0_area += cell.area;
                        else p1_area += cell.area;
                    }
                }
            }
            
            // Select cell to move
            int move_from = (p0_area > p1_area) ? 0 : 1;
            int cell_to_move = getMaxGainCell(move_from);
            if (cell_to_move == -1) break;
            
            // Update area balance proactively
            if (fm_cells_vec[cell_to_move].partition == 0) {
                p0_area -= fm_cells_vec[cell_to_move].area;
                p1_area += fm_cells_vec[cell_to_move].area;
            } else {
                p0_area += fm_cells_vec[cell_to_move].area;
                p1_area -= fm_cells_vec[cell_to_move].area;
            }
            
            moved_cells.push_back(cell_to_move);
            moveCell(cell_to_move);
            updateNeighborGains(cell_to_move);
            
            cut = incrementalUpdateCut(cell_to_move, cut);
            cut_history.push_back(cut);
            
            // Log less frequently for better performance
            if (step % 10000 == 0) {
                cout << "Pass " << pass << ", Step " << step << ": cut = " << cut << endl;
            }
            
            // Track best solution in this pass
            if (cut < pass_best_cut) {
                pass_best_cut = cut;
                pass_best_step = moved_cells.size() - 1;
                no_improvement_count = 0; // Reset no improvement count
            } else {
                no_improvement_count++;
                if (no_improvement_count >= max_no_improvement) {
                    cout << "Early stopping after " << no_improvement_count
                         << " steps without improvement" << endl;
                    break;  // Stop if no improvement for too many steps
                }
            }
        }
        
        // Restore best solution from this pass
        for (int i = moved_cells.size() - 1; i > pass_best_step; i--) {
            moveCell(moved_cells[i]); // Move back
            cut = incrementalUpdateCut(moved_cells[i], cut);
        }
       
        cout << "Pass " << pass << " complete. Best cut: " << pass_best_cut << endl;
        
        // Update global best solution
        if (cut < best_cut) {
            best_cut = cut;
            for (int i = 0; i < fm_cells_vec.size(); i++) {
                best_partition[i] = fm_cells_vec[i].partition;
            }
        } else {
            // No improvement in this pass, stop
            cout << "No improvement in this pass, stopping." << endl;
            break;
        }
    }

    // Apply the best partition found
    for (int i = 0; i < fm_cells_vec.size(); i++) {
        fm_cells_vec[i].partition = best_partition[i];
    }

    cout << "Best cut found: " << best_cut << endl;

    ofstream fout("partition_result.txt");
    for (int i = 0; i < fm_cells_vec.size(); i++) {
        fout << fm_cells_vec[i].name << " " << fm_cells_vec[i].partition << "\n";
    }
    fout.close();
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;
    cout << "Elapsed time: " << elapsed.count() << " seconds" << endl;
    return 0;
}

