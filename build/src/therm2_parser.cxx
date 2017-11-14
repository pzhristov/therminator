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
    printf("%s <input_file> <output_file>\n", progName);
}

int main(int argc, char* argv[])
{
    printf("Therminator2 parser version 2.0\n");
    if(argc < 3)
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
    
    string inputFileName = firstArg;
    string outputFileName = argv[2];

    Therminator2ToHepMCParser* parser = new Therminator2ToHepMCParser(inputFileName, outputFileName);
    parser->Run();
    
    return 0;
}
