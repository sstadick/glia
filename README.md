glia
========
### a Graph/Smith-Waterman (partial order) aligner/realigner

#### Erik Garrison <erik.garrison@bc.edu>
#### Deniz Kural <deniz.kural@gmail.com>

Installation Instructions
----------

Make sure you download the submodules by adding the `recursive` flag to the clone command:

    git clone --recursive https://github.com/sstadick/glia.git

Then build with a call to make:

    cd glia
    make

You'll need cmake to compile bamtools, which is a submoduled dependency.  ZLIB is also required, but is typically installed system-wide on development servers.

Notes
--------
- Glia is case sensitive. So your reference and read sequences must be in the same case otherwise you will get an error that looks like
    error:[gssw] Stuck in main matrix!

    glia: src/gssw.c:1199: gssw_alignment_trace_back_byte: Assertion `0' failed.

    Aborted (core dumped)
- The vcf file that it takes must be bgzip-ed and indexed with tabix. It must also have a sample column, even if it's a dumby column
- It appears to be unable to handle unmapped reads where both of a pair are unmapped
    HWI-M02090:334:000000000-AHV6V:1:1101:11471:2107#CGATG  77      *       0       0       *       *       0       0       CACCTTTAGTGACTGCACGTTTGGCAAGCAGTGGTCAGGCTGCCCTGGGATCGGGTAAAGAGTCTCAGAGCTTGGCCCCAAGCCTCTCTAGGGTAGCAGGGGCCCAGTCAGTCCAGCTTCACAGCCTGGCGCCTGCTGAGCTGACCCCCN      BBABBFFBFFFFGGDGFGEFGFEEHHHGGHFGFFEFFF2BGF2GFHHEACGGFA0AEF55B2FGBGFFHF33FG?GGHGG?GECGHHGH4GHHBEFFHGEEEFCEGFHHGFHHHFBF3EGHHHEGEHAEFBEGGFGFCF0F@FFHBGGGG      AS:i:0  XS:i:0  BC:Z:CGATG
    HWI-M02090:334:000000000-AHV6V:1:1101:11471:2107#CGATG  141     *       0       0       *       *       0       0       GACCCAGAGAGGGAGAAGATCGACTTGGAGTCTAGCATGCAGGAGGTGCAGGAAGGCGAGCACGCAGACGGAGGCTTGCAGGATGTCAAGGTTGGGCCACGCTGAGGAGCGGCGCATTTTTTCCTCTTGCCTTGGAGCCCGGCCTTCACT      11A>AA11>11A11111011BA000BF1B01B12AF1DDG1B0C//EA1BFA/0B/G//AA/0>////BE/?/>EF0EG110/11122>11BG0E/CG/F/C//<CFBB/////>-<.=DG-GHHDD000<0D//<</CECCC#######      AS:i:0  XS:i:0  BC:Z:CGATG


Example usage
--------

glia consumes a VCF, a target region, and a string to generate an alignment of any string against the variant graph defined by the VCF.  For instance calling:

    glia -f ~/human_g1k_v37.fasta -t 20:62900077-62902077 -v variants.vcf.gz \
         -s AAATGTAAACATTTTATAGGGGATTCCCCTAAAAACAAAAAAACTTTCTGGGAAAGATTTTTCAAAAAATAAAA

Will report the alignment of this string against the variant graph in the region 20:62900077-62902077.  This mode is provided for debugging purposes.

Streaming local realignment
--------

glia's main use is as a local realigner.  It will realign reads to a set of known (or putative) variants in a VCF, both consuming and producing an ordered stream of BAM alignments.  As such, it can be used immediately prior to variant calling with standard methods.  In this case, we use freebayes:

    <sample.bam glia -Rr -w 1500 -S 200 -Q 200 -G 4 -f ref.fasta -v known_variants.vcf.gz \
        | freebayes -f ref.fasta --stdin >sample.vcf

Here, glia is running reads that have a soft-clip ('S' cigar element) quality sum of >200 (-S 200), or a mismatch quality sum >200 (-Q 200) or any indel (default), and only accepting alignments that have fewer than 4 embedded gaps and decrease the overall quality score sum of soft-clips and mismatches.  So as to match unaligned mates to their correct alignments, this invocation of glia will align reads to the variant graph constructed across a window of 1500bp semi-centered around the read alignment position (-w 1500).  These settings are typically sensible for realigning Illumina 70bp reads as found in the 1000 Genomes Project.

Why?
--------

The alignment of reads can be frustrated by variants, both large (e.g. structural variants) and small (e.g. SNPs, small indels).  Observations of larger variants, where the variant scale is of a large fraction of a read length or greater, can be rescued by using unaligned mates from paired sequencing.  For smaller variants, more typically contemporary aligners will soft-clip starts and ends of a read when the read contains multiple mismatches or gaps.  Soft-clipping makes sense in that the tails of reads often contain many sequencing errors, but this filtering practice can reduce sensitivity of resequencing even to small variants.

glia realigns poorly-aligned reads against a local variant graph in order to resolve these biases, provided we know about, or have a good hint that a variant potentially exists in our dataset.  Suitable input could be either a known variant set such as [dbSNP](https://www.ncbi.nlm.nih.gov/projects/SNP/) or a union set of variants such as might be produced by an [ensemble variant detection process](https://github.com/ekg/1000G-integration)).

How?
--------

glia uses an independently-developed (by Deniz Kural) version of [partial order alignment algorithm](http://bioinformatics.oxfordjournals.org/content/18/3/452.short) to realign reads to a local "variant graph," "sequence graph," or "partial order."  This structure was originally applied to the problem of multiple sequence alignment due to its favorable time and space complexity, which is approximately O(NM) where N is the length of the query and M is the total length of the partial-order graph against which we are aligning.  In glia, we apply this method to read alignment and variant detection in the context of the sequence reads generated by 2nd and 3rd-generation sequencing platforms.

Considering a multiple sequence alignment, we can construct a partially-ordered graph in which all of the input sequences are paths:

<img src="http://i.imgur.com/kZk6UJo.png" alt="partial order graph construction">

(Note that these figures are drawn from Christopher Lee's [original paper on partial order alignment](http://bioinformatics.oxfordjournals.org/content/18/3/452.short))

This is the structure which glia aligns reads against.  Provided a VCF file encodes the same structure and does not include internal conflicts, we can use it for the generation of partially-ordered, directed acyclic graphs.

In partial order (or graph-Smith-Waterman) alignment, the standard dynamic-programming alignment algorithm is generalized to allow alignment across edges of this graph.  This can be understood as a modification of the recurrence relation used to define the score distribution across the alignment matrix to account for the score on the upstream side of inbound links.  When we initialize a new matrix, we take the score and identity of the node with the maximum alignment score in the same position in the previous node matrix.

<img src="http://i.imgur.com/uZXH9MW.png" alt="partial order alignment">

This then allows us to trace back the alignment across the edges of the graph.  In practice, glia uses this information to determine if the graph can provide a better alignment of a given read.  When the new alignment has better metrics in terms of quality/mismatch or soft-clip, then it is accepted, "flattened" from the graph back into the reference space, and emitted as a BAM record for downstream processing.  If the new alignment is not an improvement, or if there are other problems, then the original alignment is provided as-is.

Help!!!?
--------

Email <erik.garrison@bc.edu> with any questions or concerns.  Please report bugs via the bugtracking system in github.
