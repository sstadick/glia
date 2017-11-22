//
//  parameters.h
//  Based on marthlab/ekg param use of getopt
//  Created by Deniz Kural on 2/19/13.
//


#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <getopt.h>
#include <stdlib.h>

using namespace std;

int levenshteinDistance(const std::string source, const std::string target);


// Command line parameters Class
class Parameters {

    friend ostream &operator<<(ostream &out, const Parameters &p);

public:

    // i/o parameters:
    string read_input;           // -s --sequence
    string fastq_file;          // -q --fastq-file
    string fasta_reference;     // -f --fasta-reference
    string target;             // -t --target
    string vcf_file;            // -v --vcf-file
    string outputFile;          // -o --bam-output

    // operation parameters
    bool useFile;               // -x --fastq-file
    bool alignReverse;          // -r --reverse-complement
    bool realign_bam;
    bool flatten_alignments;
    int flatten_flank;
    bool flat_input_vcf;
    bool variants_only;

    // realignment parameters
    int dag_window_size;

    // alignment parameters
    int match;
    int mism;
    int gap_open;
    int gap_extend;

    // functions
    Parameters(int argc, char ** argv);
    void usage(char ** argv);
    void simpleUsage(char ** argv);

    //reporting
    string commandline;

    bool display_backtrace;
    bool display_all_nodes;
    bool display_dag;
    bool display_alignment;

    bool debug;
    bool dry_run;
    bool only_realigned;
    bool unsorted_output;

    // trigger realignment
    int mismatch_qsum_threshold;
    int softclip_qsum_threshold;
    int gap_count_threshold;
    int gap_length_threshold;

    // accept realignment only if <
    int mismatch_qsum_max;
    int softclip_qsum_max;
    int gap_count_max;

};


#endif /* defined(PARAMETERS_H) */
