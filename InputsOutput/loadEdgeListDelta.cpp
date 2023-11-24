#include "input_output.h"
#include "defs.h"
#include "sstream"
#include "utilityStringTokenizer.hpp"

void count_graph(const char * fileName, long * Vout, long * Eout) {
    long NV=0, NE=0;
    long nv1, nv2;
    FILE *file = fopen(fileName, "r");
    while(!feof(file))
    {
        fscanf(file, "%ld %ld", &nv1, &nv2);
        if(nv1 > NV) NV = nv1;
        if(nv2 > NV) NV = nv2;
        NE++;
    }
    fclose(file);
    *Vout = NV;
    *Eout = NE;
}

void parse_Delta(graph * G_out, const char *deltaFileName, const graph * G_in) {
    FILE *file = fopen(deltaFileName, "r");
    if (file == NULL) {
        printf("Cannot open the input file: %s\n",deltaFileName);
        exit(1);
    }

    // read number of vertices and edges, the first line
    long NV=0, NE=0;
    count_graph(deltaFileName, &NV, &NE);
    printf("|V|= %ld, |E|= %ld \n", NV, NE);

    edge *tmpEdgeList = (edge *) malloc( NE * sizeof(edge)); //Every edge stored ONCE
    assert( tmpEdgeList != NULL);
    long Si, Ti;
    double Twt = 1.0;
    for (long i = 0; i < NE; i++) {
        fscanf(file, "%ld %ld", &Si, &Ti);
        Si -- ; Ti --;
        assert((Si >= 0)&&(Si < NV));
        assert((Ti >= 0)&&(Ti < NV));
        tmpEdgeList[i].head   = Si;          //The S index
        tmpEdgeList[i].tail   = Ti;          //The T index: Zero-based indexing
        tmpEdgeList[i].weight = 1.0; //Make it positive and cast to Double?

    }
    fclose(file); 
 
    long *edgeListPtr = (long *)  malloc((NV+1) * sizeof(long));
    assert(edgeListPtr != NULL);
    edge *edgeList = (edge *) malloc( NE * sizeof(edge)); //Every edge stored once
    assert( edgeList != NULL);
    
#pragma omp parallel for
    for (long i=0; i <= NV; i++)
        edgeListPtr[i] = 0; //For first touch purposes
    
    //////Build the EdgeListPtr Array: Cumulative addition
#pragma omp parallel for
    for(long i=0; i<NE; i++) {
        __sync_fetch_and_add(&edgeListPtr[tmpEdgeList[i].head+1], 1); //Leave 0th position intact
    }
    for (long i=0; i<NV; i++) {
        edgeListPtr[i+1] += edgeListPtr[i]; //Prefix Sum:
    }
    
    //Keep track of how many edges have been added for a vertex:
    printf("About to allocate for added vector: %ld\n", NV);
    long  *added  = (long *)  malloc( NV  * sizeof(long));
    printf("Done allocating memory fors added vector\n");
    assert( added != NULL);
#pragma omp parallel for
    for (long i = 0; i < NV; i++)
        added[i] = 0;
    
    printf("[!] About to build edgeList...\n");
    //Build the edgeList from edgeListTmp:
#pragma omp parallel for
    for(long i=0; i<NE; i++) {
        long head      = tmpEdgeList[i].head;
        long tail      = tmpEdgeList[i].tail;
        double weight  = tmpEdgeList[i].weight;
        
        long Where = edgeListPtr[head] + __sync_fetch_and_add(&added[head], 1);
        edgeList[Where].head = head;
        edgeList[Where].tail = tail;
        edgeList[Where].weight = weight;
    }

    G_out->sVertices    = NV;
    G_out->numVertices  = NV;
    G_out->numEdges     = NE;
    G_out->edgeListPtrs = edgeListPtr;
    G_out->edgeList     = edgeList;
    
    free(tmpEdgeList);
    free(added);
    
    
    printf("[!] parse Delta done ! \n");
}
