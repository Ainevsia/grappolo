#include "defs.h"
#include "input_output.h"
#include "basic_comm.h"
#include "basic_util.h"

using namespace std;

int main(int argc, char** argv) {
    
    //Parse Input parameters:
    clustering_parameters opts;
    if (!opts.parse(argc, argv)) {
        return -1;
    }
    int nT = 1; //Default is one thread
#pragma omp parallel
    {
        nT = omp_get_num_threads();
    }
    
    double time1, time2;
    graph* G = (graph *) malloc (sizeof(graph));
    char *inFile = (char*) opts.inFile;
    parse_UndirectedEdgeList(G, inFile);
    int threadsOpt = 1;
    
    int replaceMap = 0;
    if(  opts.basicOpt == 1 )
        replaceMap = 1;
    
    long NV = G->numVertices;

    // Datastructures to store clustering information
    long *C_orig = (long *) malloc (NV * sizeof(long)); assert(C_orig != 0);
    
    // The original version of the graph
    graph* G_orig = (graph *) malloc (sizeof(graph)); 
    duplicateGivenGraph(G, G_orig);
    
#pragma omp parallel for
    for (long i=0; i<NV; i++) {
        C_orig[i] = -1;
    }
    runMultiPhaseBasic(G, C_orig, opts.basicOpt, opts.minGraphSize, opts.threshold, opts.C_thresh, nT,threadsOpt);
    
    //Check if cluster ids need to be written to a file:
    if( opts.output ) {
        char outFile[256];
        sprintf(outFile,"%s_clustInfo", opts.inFile);
        printf("Cluster information will be stored in file: %s\n", outFile);
        FILE* out = fopen(outFile,"w");
        for(long i = 0; i<NV;i++) {
            fprintf(out,"%ld\n",C_orig[i]);
        }
        fclose(out);
    }
    
    free(C_orig);

    // now start dealing with delta files, like delta-screaning 
    for (size_t i = 0; i < opts.nDelta; i++)
    {
        string sDeltaFileName = opts.sDeltaFilePrefix + to_string(i) + string(".txt");
        graph* G_new = (graph *) malloc (sizeof(graph));
        parse_Delta(G_new, sDeltaFileName.c_str(), G_orig);

    }
    
    




    return 0;
}
