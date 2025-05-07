# Fiduccia-Mattheyses-partitioning
To implement and experiment the Fiduccia-Mattheyses partitioning algorithm for gate-level designs. It allows the user to select what type of area definition they would like to use. They can choose the number of gates definition or area of gates defintion for area.  
## How to Use

### Input Files
he program expects benchmark files in ISPD format (superblue18) with the following components:
1. **`.aux` file**: Specifies the names of the other input files.
2. **`.nodes` file**: Contains information about the cells (name, width, height).
3. **`.nets` file**: Describes the nets and their connections to cells.
4. **`.pl` file**: Specifies the placement of cells (x, y coordinates).
5. **`.scl` file**: Defines the rows and their properties.

### File Path Configuration
The program assumes that all input files are located in a directory relative to the current working directory. By default, the program looks for the `superblue18.aux` file in the `superblue18 2` directory. The 

#### Changing File Paths
To change the file paths:
1. Locate the `parseAux` function in `main.cpp`.
2. Modify the `basePath` variable to point to the directory containing your input files:
   ```cpp
   string basePath = "C:/path/to/your/files/";
   ```
3. Ensure that all input files specified in the `.aux` file are present in the directory. It is currently configured to look for the superblue18 2 folder placed inside the main directory of the code repository which is included in this repository.

### Running the Program
1. Compile the program using cmake and the included cmakelists.txt
2. Run the resulting program
3. When prompted, choose the area definition:
   - Enter `1` for gate count (each gate counts as 1).
   - Enter `2` for gate area (width × height).

### Output
The program generates an output file named `partition_result.txt`, which contains the partitioning results. Each line in the file specifies a cell name and its assigned partition (0 or 1).

### Example
Given the following `.aux` file:
```
RowBasedPlacement : example.nodes example.nets example.wts example.pl example.scl example.shapes example.route
```
Ensure the following files are present in the specified directory:
- `example.nodes`
- `example.nets`
- `example.pl`
- `example.scl`

Run the program, and it will output the partitioning results to `partition_result.txt`.

### Notes
- The program outputs logs to the console, including the number of cells, nets, and rows parsed, as well as the progress of the FM algorithm.
- Ensure that the input files are correctly formatted and match the expected structure.

### Troubleshooting
- If the program cannot find the `.aux` file, verify the `basePath` variable and ensure the file exists in the specified directory.
- If parsing errors occur, check the formatting of the input files.