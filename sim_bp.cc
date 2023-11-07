

// BranchPrediction.cpp : This file contains the 'main' function. Program execution begins and ends there.


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started:
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file


#define _CRT_SECURE_NO_DEPRECATE
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include "sim_bp.h"
using namespace std;

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim bimodal 6 gcc_trace.txt
    argc = 4
    argv[0] = "sim"
    argv[1] = "bimodal"
    argv[2] = "6"
    ... and so on
*/

unsigned long int     pc_addr = 0;
unsigned long int     m = 0;
unsigned long int     n = 0;
unsigned long int     l = 0;
unsigned long int     number_Of_Entries = 0;
unsigned long int     mispredictions = 0;
unsigned long int     numberOfPredictions = 0;
double                mispredictionRate = 1;
unsigned long int     gh_register = 0;
unsigned long int     bimodal_Number_Of_Enteries = 0;
unsigned long int     gshare_Number_Of_Enteries = 0;
unsigned long int     hybrid_Number_of_Entries = 0;
unsigned long int     hybrid_mispredictions = 0;

bp_table* bp_table_entry; // here bp_table_entry[i] . So we define a 1 dimensional array which holds the structure of PC addresses being referenced.

bp_table* bimodal_table;
bp_table* gshare_table;
bp_table* chooser_table;


void printBinary(int number) {
    for (int i = sizeof(number) * 8 - 1; i >= 0; i--) {
        int mask = 1 << i;
        printf("%d", (number & mask) ? 1 : 0);
    }
    printf("\n");
}


void initializeBranchPredictionEntryTable(int entries, bp_table*& table) {
    table = new bp_table[entries];

    if (table != NULL) {
        for (int i = 0; i < entries; i++) {
            // Initialize default values here
            table[i].counter_value = 2; // This is a weakly taken case.
        }
    }
    else {
        printf("Memory allocation failed for branch prediction table.\n");
        exit(EXIT_FAILURE);
    }
}

void initializeChooserTable(int entries) {
    chooser_table = new bp_table[entries];

    if (chooser_table != NULL) {
        for (int i = 0; i < entries; i++) {
            // Initialize default values here
            chooser_table[i].counter_value = 1; // This is a weakly taken case.
        }
    }
    else {
        // Handle memory allocation failure
        // You can print an error message or take appropriate action.
        //printf("Memory allocation failed for bp_table_entry.\n");

    }
}



void initializeBranchPredictionEntryTable(int entries) {
    bp_table_entry = new bp_table[entries];

    if (bp_table_entry != NULL) {
        for (int i = 0; i < entries; i++) {
            // Initialize default values here
            bp_table_entry[i].counter_value = 2; // This is a weakly taken case.
        }
    }
    else {
        // Handle memory allocation failure
        // You can print an error message or take appropriate action.
        printf("Memory allocation failed for bp_table_entry.\n");

    }
}

void cleanupBranchPredictionEntryTable() {
    if (bp_table_entry != NULL) {
        free(bp_table_entry);
        bp_table_entry = NULL;
    }
}

void generatePCAddress(unsigned long int address, unsigned long int index_bits, unsigned long int entries) {
    pc_addr = (address >> 2);
    m = (pc_addr) & ((1 << index_bits) - 1);    //this is the index from PC address
    /*entries = static_cast<unsigned long  int>(std::pow(2, index_bits));*/
    //number_Of_Entries = static_cast<unsigned long  int>(std::pow(2, index_bits));


}

void getPCAddress(unsigned long int address)
{
    pc_addr = (address >> 2);
}

unsigned long int getIndexBiModal(unsigned long int address, unsigned long int index_bits, unsigned long int entries) {
    unsigned long int index = (address >> 2) & ((1 << index_bits) - 1);    //this is the index from PC address
    return index;
}

unsigned long int getIndexHybrid(unsigned long int address, unsigned long int index_bits) {
    unsigned long int index = (address >> 2) & ((1 << index_bits) - 1);    //this is the index from PC address
    return index;
}


unsigned long int getIndex(unsigned long int pc_address, unsigned long int m1, unsigned long int n)

{
    //Extract last M digits
    unsigned long int MBits = (pc_address >> 2) & ((1 << m1) - 1);
    //Extract the upeer address M-N  digits
    unsigned long int  MminusNBits = (MBits >> (m1 - n));
    int exor_result = gh_register ^ (MminusNBits);
    unsigned long int NBits = MBits & ((1 << (m1 - n)) - 1);
    unsigned long int index = (exor_result << (m1 - n) | NBits);

    // printf("the Index are : %d\n", index);
    return index;
}



void  updateGlobalHistoryRegister(unsigned long int n, char branchOutcome, unsigned long int globalHistoryRegister)
{
    if (branchOutcome == 't')
    {
        globalHistoryRegister = (globalHistoryRegister >> 1) | (1 << (n - 1));
        globalHistoryRegister = (globalHistoryRegister) & ((1 << n) - 1);

        //gh_register = globalHistoryRegister;
    }
    if (branchOutcome == 'n')
    {
        globalHistoryRegister = (globalHistoryRegister >> 1) | (0 << (n - 1));
        globalHistoryRegister = (globalHistoryRegister) & ((1 << n) - 1);
        // Shift the bits to the left by 1 and ensure the leftmost bit is 0
       // globalHistoryRegister = ((globalHistoryRegister << 1) & ((1 << n) - 1));
        //gh_register = globalHistoryRegister;
    }
    gh_register = globalHistoryRegister;

   /* printBinary(gh_register);
    printf("\n the binary value is %d", gh_register);*/
    // printf("the gbhr is : %d \n", gh_register);
}



void updateGlobalHistoryPredictionTable_gshare_hybrid(unsigned long int index, char branchOutcome, unsigned long int n) {

    // Step 1: Make a prediction
    bool predictionTaken = gshare_table[index].counter_value >= 2;

    // Step 3: Update the branch predictor based on the actual outcome
    if (branchOutcome == 't') {
        // Taken branch
        if (predictionTaken) {
            // Correct prediction, no update needed
            if (gshare_table[index].counter_value == 3) {}
            else {
                gshare_table[index].counter_value++;
            }
        }
        else {
            // Misprediction, increment the counter and saturate at 3
            if (gshare_table[index].counter_value < 3) {
                gshare_table[index].counter_value++;
               // mispredictions++;
            }
        }
    }
    else {
        // Not-taken branch
        if (predictionTaken) {
            // Misprediction, decrement the counter and saturate at 0
            if (gshare_table[index].counter_value == 0) {}
            else {
                gshare_table[index].counter_value--;
            }

           // mispredictions++;
        }
        else {
            // Correct prediction, no update needed
            if (gshare_table[index].counter_value == 0) {}
            else {
                gshare_table[index].counter_value--;
            }

        }


        if ((predictionTaken && branchOutcome == 'n') || (!predictionTaken && branchOutcome == 't')) {
            // Misprediction occurred
            // Increment the mispredictions counter here
            mispredictions++;
        }
    }

   // printf(" the index for gshare is %d and counter value is %d ", index, gshare_table[index].counter_value);
    // Step 4: Update the global branch history register
    //updateGlobalHistoryRegister(n, branchOutcome, gh_register);
}

void updatePredictionTable_bimodal_hybrid(unsigned long int index, unsigned long int pc_address, unsigned long int entries, char branchOutcome) {
    // Step 1: Make a prediction
    bool predictionTaken = bimodal_table[index].counter_value >= 2;

    // Step 3: Update the branch predictor based on the actual outcome
    if (branchOutcome == 't') {
        // Taken branch
        if (predictionTaken) {
            // Correct prediction, no update needed
            if (bimodal_table[index].counter_value == 3) {}
            else {
                bimodal_table[index].counter_value++;
            }
        }
        else {
            // Misprediction, increment the counter and saturate at 3
            if (bimodal_table[index].counter_value < 3) {
                bimodal_table[index].counter_value++;
               // mispredictions++;
            }
        }
    }
    else {
        // Not-taken branch
        if (predictionTaken) {
            // Misprediction, decrement the counter and saturate at 0
            if (bimodal_table[index].counter_value == 0) {}
            else {
                bimodal_table[index].counter_value--;
            }

          //  mispredictions++;
        }
        else {
            // Correct prediction, no update needed
            if (bimodal_table[index].counter_value == 0) {}
            else {
                bimodal_table[index].counter_value--;
            }

        }
    }

    if ((predictionTaken && branchOutcome == 'n') || (!predictionTaken && branchOutcome == 't')) {
        // Misprediction occurred
        // Increment the mispredictions counter here
        mispredictions++;
    }
}


// Function to update the branch prediction table and track mispredictions
void updatePredictionTable(unsigned long int index, unsigned long int pc_address, unsigned long int entries, char branchOutcome, unsigned long int N) {
    // Step 1: Make a prediction
    bool predictionTaken = bp_table_entry[index].counter_value >= 2;

    // Step 3: Update the branch predictor based on the actual outcome
    if (branchOutcome == 't') {
        // Taken branch
        if (predictionTaken) {
            // Correct prediction, no update needed
            if (bp_table_entry[index].counter_value == 3) {}
            else {
                bp_table_entry[index].counter_value++;
            }
        }
        else {
            // Misprediction, increment the counter and saturate at 3
            if (bp_table_entry[index].counter_value < 3) {
                bp_table_entry[index].counter_value++;
                //mispredictions++;
            }
        }
    }
    else {
        // Not-taken branch
        if (predictionTaken) {
            // Misprediction, decrement the counter and saturate at 0
            if (bp_table_entry[index].counter_value == 0) {}
            else {
                bp_table_entry[index].counter_value--;
            }

            //mispredictions++;
        }
        else {
            // Correct prediction, no update needed
            if (bp_table_entry[index].counter_value == 0) {}
            else {
                bp_table_entry[index].counter_value--;
            }

        }
    }

    if ((predictionTaken && branchOutcome == 'n') || (!predictionTaken && branchOutcome == 't')) {
        // Misprediction occurred
        // Increment the mispredictions counter here
        mispredictions++;
    }

    //  Step 4: Update the global branch history register
    if (N != 0) {
        updateGlobalHistoryRegister(N, branchOutcome, gh_register);
    }
}




int main(int argc, char* argv[])
{
    FILE* FP;               // File handler
    char* trace_file;       // Variable that holds trace file name;
    bp_params params;       // look at sim_bp.h header file for the the definition of struct bp_params
    char outcome;           // Variable holds branch outcome
    unsigned long int addr; // Variable holds the address read from input file

    if (!(argc == 4 || argc == 5 || argc == 7))
    {
        printf("Error: Wrong number of inputs:%d\n", argc - 1);
        exit(EXIT_FAILURE);
    }

    params.bp_name = argv[1];

    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    if (strcmp(params.bp_name, "bimodal") == 0)              // Bimodal
    {
        if (argc != 4)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc - 1);
            exit(EXIT_FAILURE);
        }
        params.M2 = strtoul(argv[2], NULL, 10);
        trace_file = argv[3];
        printf("COMMAND\n%s %s %lu %s\n", argv[0], params.bp_name, params.M2, trace_file);
    }
    else if (strcmp(params.bp_name, "gshare") == 0)          // Gshare
    {
        if (argc != 5)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc - 1);
            exit(EXIT_FAILURE);
        }
        params.M1 = strtoul(argv[2], NULL, 10);
        params.N = strtoul(argv[3], NULL, 10);
        trace_file = argv[4];
        printf("COMMAND\n%s %s %lu %lu %s\n", argv[0], params.bp_name, params.M1, params.N, trace_file);

    }
    else if (strcmp(params.bp_name, "hybrid") == 0)          // Hybrid
    {
        if (argc != 7)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc - 1);
            exit(EXIT_FAILURE);
        }
        params.K = strtoul(argv[2], NULL, 10);
        params.M1 = strtoul(argv[3], NULL, 10);
        params.N = strtoul(argv[4], NULL, 10);
        params.M2 = strtoul(argv[5], NULL, 10);
        trace_file = argv[6];
        printf("COMMAND\n%s %s %lu %lu %lu %lu %s\n", argv[0], params.bp_name, params.K, params.M1, params.N, params.M2, trace_file);

    }
    else
    {
        printf("Error: Wrong branch predictor name:%s\n", params.bp_name);
        exit(EXIT_FAILURE);
    }

    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if (FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    char str[2];
    //while (fscanf(FP, "%lx %s", &addr, str) != EOF)
    //{

    //    outcome = str[0];
    //    if (outcome == 't')
    //        printf("%lx %s\n", addr, "t");           // Print and test if file is read correctly
    //    else if (outcome == 'n')
    //        printf("%lx %s\n", addr, "n");          // Print and test if file is read correctly
    //    /*************************************
    //        Add branch predictor code here
    //    **************************************/


    //}
    /*************************************
            Add branch predictor code here
        **************************************/

    if (strcmp(params.bp_name, "bimodal") == 0) {
        number_Of_Entries = static_cast<unsigned long  int>(std::pow(2, params.M2)); // the total number of entries in Branch prediction Table
    }
    if ((strcmp(params.bp_name, "gshare") == 0)) {
        number_Of_Entries = static_cast<unsigned long  int>(std::pow(2, params.M1)); // the total number of entries in Branch prediction Table

    }

    if (strcmp(params.bp_name, "hybrid") == 0)
    {
        // lets do bimodal first
        bimodal_Number_Of_Enteries = static_cast<unsigned long  int>(std::pow(2, params.M2)); // the total number of entries in Branch prediction Table for bimodal
        //lets find for ghsare
        gshare_Number_Of_Enteries = static_cast<unsigned long  int>(std::pow(2, params.M1)); // the total number of enteries for branch prediction table for gshare

        hybrid_Number_of_Entries = static_cast<unsigned long  int>(std::pow(2, params.K));

        // Intilaize the tables
        // Initialize branch prediction tables
        initializeBranchPredictionEntryTable(bimodal_Number_Of_Enteries, bimodal_table);
        initializeBranchPredictionEntryTable(gshare_Number_Of_Enteries, gshare_table);
        initializeChooserTable(hybrid_Number_of_Entries);


    }

    initializeBranchPredictionEntryTable(number_Of_Entries);
    unsigned long int index = 0;
    unsigned long int bimodalIndex = 0;
    while (fscanf(FP, "%lx %s", &addr, str) != EOF) {
        numberOfPredictions++;
        outcome = str[0];
        int real_outcome = 0;

        if (outcome == 'n')
        {
            real_outcome = 0;
        }
        else
        {
            real_outcome = 1;
        }



        if (strcmp(params.bp_name, "bimodal") == 0) {
            getPCAddress(addr);
            unsigned long int index = getIndexBiModal(addr, params.M2, number_Of_Entries);
            updatePredictionTable(index, pc_addr, number_Of_Entries, outcome, params.N);

            //updatePredictionTable(index, pc_addr, number_Of_Entries, outcome);
            //generatePCAddress(addr, params.M2, number_Of_Entries);
            //updatePredictionTable(m, pc_addr, number_Of_Entries, outcome);
        }
        if ((strcmp(params.bp_name, "gshare") == 0)) {
            generatePCAddress(addr, params.M1, number_Of_Entries);


            //printBinary(gh_register);
           // printf("\nthe size of gh_register is %d", sizeof(gh_register));


           // generateBranchHistoryRegister(params.N);
            //printf("the N value is %d", params.N);
           // printf(" INDEX for gshare is %d is ", params.M1);
            //unsigned long int index = generateIndex(addr, params.M1, params.N);
            unsigned long int index = getIndex(addr, params.M1, params.N);
            updatePredictionTable(index, pc_addr, number_Of_Entries, outcome, params.N);
            //updateGlobalHistoryPredictionTable_gshare(index, outcome, params.N);
        }





        if (strcmp(params.bp_name, "hybrid") == 0) {
            getPCAddress(addr);
            bool prediction = true;
            m = getIndexBiModal(addr, params.M2, bimodal_Number_Of_Enteries);
            //bimodalIndex = getIndexBiModal(addr, params.M2, bimodal_Number_Of_Enteries);
            //printf("the address is %x\n", addr);
            //printf("the index for bimodal is %d\n", m);

            bool prediction_bimodal = bimodal_table[m].counter_value >= 2;

            /*if (prediction_bimodal == real_outcome)
            {
                mispredictions++;
            }*/


            // n = getIndex(addr, params.M1, params.N);
             index = getIndex(addr, params.M1, params.N);
            n = params.N;// vignesh made at 2:10 pm Nov 6
            //printf("the indes for gshare is %d\n", n);

            bool prediction_gshare = gshare_table[index].counter_value >= 2;

            l = getIndexHybrid(addr, params.K);
            //printf("the index for hybrid is %d\n", l);

            if (chooser_table[l].counter_value >= 2)
            {
                prediction = gshare_table[index].counter_value >= 2;
                //updateGlobalHistoryPredictionTable_gshare_hybrid(n, outcome, params.N);
                updateGlobalHistoryPredictionTable_gshare_hybrid(index, outcome, params.N);
                if (prediction != real_outcome)
                {
                    hybrid_mispredictions++;
                }
            }
            else
            {
                prediction = bimodal_table[m].counter_value >= 2;
                updatePredictionTable_bimodal_hybrid(m, pc_addr, bimodal_Number_Of_Enteries, outcome);
                if (prediction != real_outcome)
                {
                    hybrid_mispredictions++;
                }
            }

           // printf(" gshare index is %d and gshare counter is %d \n", n, gshare_table[n].counter_value);
            //printf("the bimodal index is %d and bimodal counter is %d \n", m, bimodal_table[m].counter_value);
            updateGlobalHistoryRegister(n, outcome, gh_register);
           // printBinary(gh_register);


            //printf("the index for bimodal is %d and ghregister is %d\n", m, gh_register);


            if ((prediction_bimodal != real_outcome) && (prediction_gshare == real_outcome))
            {
                if (chooser_table[l].counter_value < 3)
                {
                    chooser_table[l].counter_value++;
                }
            }
            else if ((prediction_bimodal == real_outcome) && (prediction_gshare != real_outcome))
            {
                if (chooser_table[l].counter_value > 0)
                {
                    chooser_table[l].counter_value--;
                }
            }

            //printf(" the index is %d and counter value is %d \n", l, chooser_table[l].counter_value--);


        }

        /*printf("=%d  ", numberOfPredictions - 1);
        printf("      address %x\n", addr);
        printf("      ghare  %d counter  %d \n", index, gshare_table[index].counter_value--);
        printf("      bimodal is %d counter %d \n", m, bimodal_table[m].counter_value--);
        printf("      chosser %d counter %d \n\n", l, chooser_table[l].counter_value--);*/

    }



    if (strcmp(params.bp_name, "bimodal") == 0) {
        cout << "OUTPUT" << endl;

        if (numberOfPredictions != 0) {
            mispredictionRate = static_cast<double>(mispredictions) / numberOfPredictions * 100.0;
        }

        cout << left << setw(26) << "number of predictions:" << right << setw(10) << numberOfPredictions << endl;
        cout << left << setw(26) << "number of mispredictions:" << right << setw(10) << mispredictions << endl;
        cout << left << setw(26) << "misprediction rate:" << fixed << setprecision(2) << right << setw(10) << mispredictionRate << "%" << endl;

        cout << "FINAL BIMODAL CONTENTS" << endl;

        // Print the Branch History Table
        for (unsigned long int i = 0; i < number_Of_Entries; i++) {
            cout << setw(3) << i << "\t" << setw(1) << bp_table_entry[i].counter_value << endl;
        }
    }



    if (strcmp(params.bp_name, "gshare") == 0) {
        cout << "OUTPUT" << std::endl;

        if (numberOfPredictions != 0) {
            mispredictionRate = static_cast<double>(mispredictions) / numberOfPredictions * 100.0;
        }

        cout << std::left << std::setw(26) << "number of predictions:" << std::right << std::setw(10) << numberOfPredictions << std::endl;
        cout << std::left << std::setw(26) << "number of mispredictions:" << std::right << std::setw(10) << mispredictions << std::endl;
        cout << std::left << std::setw(26) << "misprediction rate:" << std::fixed << std::setprecision(2) << std::right << std::setw(10) << mispredictionRate << "%" << std::endl;


        cout << "FINAL GSHARE CONTENTS" << std::endl;
        // Print the Branch History Table
        for (unsigned long int i = 0; i < number_Of_Entries; i++) {
            cout << std::setw(3) << i << "\t" << std::setw(1) <<bp_table_entry[i].counter_value << std::endl;
        }
    }



    if (strcmp(params.bp_name, "hybrid") == 0) {
        cout << "OUTPUT" << endl;

        if (numberOfPredictions != 0) {
            mispredictionRate = static_cast<double>(hybrid_mispredictions) / numberOfPredictions * 100.0;
        }

        cout << left << setw(26) << "number of predictions:" << right << setw(10) << numberOfPredictions << endl;
        cout << left << setw(26) << "number of mispredictions:" << right << setw(10) << hybrid_mispredictions << endl;
        cout << left << setw(26) << "misprediction rate:" << fixed << setprecision(2) << right << setw(10) << mispredictionRate << "%" << endl;

        cout << "FINAL CHOOSER CONTENTS" << endl;
        // Print the Branch History Table
        for (unsigned long int i = 0; i < hybrid_Number_of_Entries; i++) {
            cout << setw(3) << i << "\t" << setw(1) << chooser_table[i].counter_value << endl;
        }


         cout << "FINAL GSHARE CONTENTS" << endl;
        // Print the Branch History Table
        for (unsigned long int i = 0; i < gshare_Number_Of_Enteries; i++) {
            cout << setw(3) << i << "\t" << setw(1) << gshare_table[i].counter_value << endl;
        }
        
        cout << "FINAL BIMODAL CONTENTS" << endl;

        // Print the Branch History Table
        for (unsigned long int i = 0; i < bimodal_Number_Of_Enteries; i++) {
            cout << setw(3) << i << "\t" << setw(1) << bimodal_table[i].counter_value << endl;
        }

       
    }


    cleanupBranchPredictionEntryTable();
    return 0;
}
