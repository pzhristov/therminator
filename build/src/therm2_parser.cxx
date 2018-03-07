//Therminator2ToHepMCParser
//Written by Maciej Buczynski
//Inspired by AmptToHepMCParser by Redmer Alexander Bertens

#include "Therminator2ToHepMCParser.h"
#include <cstdio>
#include <iostream>
#include <string>
#include <stdlib.h>

using std::cout;
using std::endl;
using std::string;

void printUsage(char* progName)
{
    printf("Converts Therminator2 text output file to the HepMC format. Usage:\n");
    printf("%s <input_file> <output_file> [-d]\n", progName);
    printf("-d\tDisable pseudo-particles. The output file won't contain additional linking particles. ");
    printf("However, the positions of primary particles won't be saved.\n");
}

int main(int argc, char* argv[])
{
    printf("Therminator2 parser version 2.1\n");
    if(argc < 3 || argc > 4)
    {
        printUsage(argv[0]);
        return 1;
    }

    string firstArg = argv[1];
    if(firstArg == "-h" || firstArg == "--help" || argc<3)
    {
        printUsage(argv[0]);
        return 1;
    }
    
    bool savePrimordialPositions = true;
    
    if(argc == 4)
    {
        string extraParam = argv[3];
        if(extraParam == "-d")
        {
            savePrimordialPositions = false;
            printf("Disabling pseudo-particles.\n");
        }
        else
        {
            printf("Unknown parameter: %s, quitting...", argv[3]);
            return 1;
        }
    }
    
    string inputFileName = firstArg;
    string outputFileName = argv[2];

    Therminator2ToHepMCParser* parser = new Therminator2ToHepMCParser(inputFileName, outputFileName, savePrimordialPositions);
    parser->Run();
    
    return 0;
}
