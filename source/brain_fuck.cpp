#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>

using namespace std;

// Count the number of brainfuck operators in a given instruction set
int count_operators(string instructions) {
    string operators = "+-./<>[]";
    int operator_count = 0;

    for(auto instruction : instructions) {
        if(operators.find(instruction) != string::npos)
            operator_count += 1;
    }

    return operator_count;
}

// Get input, with some error checking
char get_input() {
    cout << endl << "> ";

    string input;
    getline(cin, input);

    if(!cin.good())
        throw -1;

    else if(input.empty()) {
        cerr << "Input error";
        throw -1;
    }

    return input[0];
}

int main(int argument_count, char *argument_vector[]) {

    // For readability's sake, add a newline
    cout << endl;

    try {

        // Check there's actually some brainfuck to run
        if(argument_count < 2) {
            cerr << "No arguments provided";
            throw -1;
        }

        // Declare the user-define-able variables
        bool verbose = false;
        string output_file;
        int cell_limit = 256;

        // Start the timer
        auto start_time = chrono::system_clock::now();

        // This string holds the instructions, read from a file, or input
        // directly as a command-line argument
        string instructions;

        // Parse the command line arguments
        // -f [file name] specify an input file
        // -l [cell limit] limit the number of cells which the program can use
        // (default 128)
        // -v specify that the output should be verbose (shows information about
        // the program)
        for(int index = 1; index < argument_count; ++ index) {

            string argument = argument_vector[index];

            // Read the instructions from a file
            if(argument == "-f") {
                if(index + 1 >= argument_count || !instructions.empty()) {
                    cerr << "Both file and literal instructions provided";
                    throw -1;
                }

                index += 1;
                string file_name = argument_vector[index];
                ifstream file(file_name);
                if(!file.is_open()) {
                    cerr << "Couldn't open file: " << file_name;
                    throw -1;
                }

                file.seekg(0, ios::end);
                instructions.reserve(file.tellg());
                file.seekg(0, ios::beg);
                instructions.assign(istreambuf_iterator<char>(file),
                        istreambuf_iterator<char>());
                file.close();
            }

            // Handle a provided cell limit
            else if(argument == "-l") {
                if(index + 1 >= argument_count) {
                    cerr << "No value provided after cell limit flag";
                    throw -1;
                }

                index += 1;
                try {
                    cell_limit = stoi(argument_vector[index]);
                }
                catch(...) {
                    cerr << "Cell limit value non-parse-able";
                    throw -1;
                }
            }

            // Handle the verbosity flag
            else if(argument == "-v")
                verbose = true;

            // If it isn't a flag, and the instructions aren't empty, that means
            // a file has already been loaded as the instruction set -- and
            // the user shouldn't have also provided instructions as an
            // argument
            else if(!instructions.empty()) {
                cerr << "Both file and literal instructions provided";
                throw -1;
            }

            // If the input isn't a flag, it's instructions provided as command-
            // line arguments
            else
                instructions += argument;
        }

        // The stack and pointer are central to brainfuck functionality, it's
        // the pseudo-memory which is manipulated by the code the user provides
        map<int, char> stack;
        int pointer = 0;

        // Stores the indices of opening brackets encountered while running
        // the instructions
        vector<int> loop_start_indices;

        // Declare some variables used when the verbosity flag is set
        int lowest_cell = 0;
        int greatest_cell = 0;
        int operations = 0;
        int left_shifts = 0;
        int right_shifts = 0;

        // Handle each instruction
        char instruction;
        for(int index = 0; index < instructions.size(); index += 1) {
            instruction = instructions[index];

            // Increment the number of operations performed (reported in verbose
            // mode)
            operations += 1;

            // Check the number of cells being used doesn't exceed the limit
            // (which could indicate that there's an endless loop)
            if(abs(greatest_cell) + abs(lowest_cell) > cell_limit) {
                cerr << "Stack size limit reached";
                throw -1;
            }

            // Increment or decrement the value of the current cell
            if(instruction == '+')
                stack[pointer] += 1;
            else if(instruction == '-')
                stack[pointer] -= 1;

            // Increment or decrement the cell pointer
            else if(instruction == '>') {
                pointer += 1;
                right_shifts += 1;
                lowest_cell = min(pointer, lowest_cell);
            }
            else if(instruction == '<') {
                pointer -= 1;
                left_shifts += 1;
                greatest_cell = max(pointer, greatest_cell);
            }

            // Write the value of the current cell (or just the integer value of
            // the cell, if it's outside the ASCII character range)
            // TODO: Decide whether to ignore such output, because it
            // technically goes against specification
            else if(instruction == '.') {
                char output;
                if(stack[pointer] < ' ' || stack[pointer] > '~')
                    output ='?';
                else
                    output = stack[pointer];

                cout << output;
            }

            // Get user input
            else if(instruction == ',')
                stack[pointer] = get_input();

            // Handle loop start characters
            else if(instruction == '[') {

                // If the cell is zero, execution needs to jump to the
                // corresponding closing bracket
                if(stack[pointer] == 0) {
                    int indentation = 1;

                    // Handle any internally nested brackets
                    while(indentation) {
                        index += 1;
                        instruction = instructions[index];

                        if(instructions[index] == '[')
                            indentation += 1;
                        else if(instructions[index] == ']')
                            indentation -= 1;
                    }
                }

                // If the cell's value is non-zero, the index of this loop-
                // starting bracket needs to be added to the loop start index
                // buffer, so it can be jumped back to later
                else if(loop_start_indices.empty() ||
                        (!loop_start_indices.empty() &&
                        loop_start_indices.back() != index))
                    loop_start_indices.push_back(index);
            }

            // Handle closing brackets
            else if(instruction == ']') {

                // If the cell's value is non-zero, jump back to the last
                // starting bracket
                if(stack[pointer]) {
                    if(!loop_start_indices.empty())
                        index = loop_start_indices.back();

                    // The buffer should never be empty (in this instance),
                    // this indicates a syntax error
                    else {
                        cout << "Syntax error";
                        throw -1;
                    }
                }

                // Otherwise, remove the last starting bracket stored from the
                // open bracket buffer
                else
                    loop_start_indices.pop_back();
            }
        }

        // Add some new-lines for readability
        cout << endl << endl;

        // If verbosity was specified, print out some statistics
        if(verbose) {

            // Count and display the number of operators in the string
            int operator_count = count_operators(instructions);
            cout << "Operator count:        " << operator_count << endl;

            cout << "Operations performed:  " << operations << endl;
            cout << "Cells used:            " << abs(greatest_cell) +
                    abs(lowest_cell) + 1 << " (" << lowest_cell << " : " <<
                    greatest_cell << ")" << endl;
            cout << "Shift operations:      " << left_shifts + right_shifts <<
                    " (" << left_shifts << " left, " << right_shifts <<
                    " right)" << endl;

            // Calculate how long the program took to run, and the average
            // number of operations performed per second
            // TODO: Find a way to make these results more repeatable -- maybe
            // also give feedback on which operators took the most time to
            // complete, or a breakdown of which loops took the longest
            auto end_time = chrono::system_clock::now();
            chrono::duration<double> elapsed_time = end_time - start_time;
            float time_in_seconds =  elapsed_time.count() *
                    chrono::seconds::period::num /
                    chrono::seconds::period::den;
            cout.precision(3);
            cout << "Time taken:            " << time_in_seconds << "s" << endl;
            cout << "Operations per second: " << operations / time_in_seconds <<
                    endl << endl;
        }
    }
    catch(...) {

        // Add some buffering after any error messages
        cerr << endl << endl;
        return -1;
    }

    return 0;
}
