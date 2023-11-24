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
    
    graph* G = (graph *) malloc (sizeof(graph));
    char *inFile = (char*) opts.inFile;
    parse_UndirectedEdgeList(G, inFile);
    int threadsOpt = 1;
    
    long NV = G->numVertices;

    long *C_orig = (long *) malloc (NV * sizeof(long)); assert(C_orig != 0);
    graph* G_orig = (graph *) malloc (sizeof(graph)); 
    duplicateGivenGraph(G, G_orig); // bakcup
    
#pragma omp parallel for
    for (long i=0; i<NV; i++) {
        C_orig[i] = -1;
    }
    runMultiPhaseBasic(G, C_orig, 
        opts.basicOpt, opts.minGraphSize, opts.threshold, 
        opts.C_thresh, nT, threadsOpt);
    G_orig->com = vector<long>(C_orig, C_orig + NV);
    if( opts.output ) {
        char outFile[256];
        sprintf(outFile,"%s_clustInfo", opts.inFile);
        printf("Cluster information will be stored in file: %s\n", outFile);
        FILE* out = fopen(outFile,"w");
        for(long i = 0; i<NV;i++) {
            fprintf(out,"%ld\n",G_orig->com[i]);

        }
        fclose(out);
    }
    free(C_orig);

    // now start dealing with delta files, like delta-screaning 
    for (size_t i = 0; i < opts.nDelta; i++)
    {
        string sDeltaFileName = opts.sDeltaFilePrefix + to_string(i) + string(".txt");
        graph* G_backup = (graph *) malloc (sizeof(graph));
        parse_Delta(G_orig, sDeltaFileName.c_str());
        NV = G_orig->numVertices;
        long *C_orig = (long *) malloc (NV * sizeof(long)); assert(C_orig != 0);
        duplicateGivenGraph(G_orig, G_backup);

#pragma omp parallel for
        for (long i=0; i<NV; i++) {
            C_orig[i] = G_backup->com[i];
        }
        // it will comsume 1st param
        runMultiPhaseBasic(G_orig, C_orig, 
            opts.basicOpt, opts.minGraphSize, opts.threshold, 
            opts.C_thresh, nT, threadsOpt);
        G_backup->com.assign(C_orig, C_orig+NV);

        G_orig = (graph *) malloc (sizeof(graph)); 
        duplicateGivenGraph(G_backup, G_orig);
    }

    if( opts.output ) {
        char outFile[256];
        sprintf(outFile,"%s_clustInfo", opts.inFile);
        printf("Cluster information will be stored in file: %s\n", outFile);
        FILE* out = fopen(outFile,"w");
        for(long i = 0; i<NV;i++) {
            fprintf(out,"%ld\n",G_orig->com[i]);
        }
        fclose(out);
    }
    
    return 0;
}
