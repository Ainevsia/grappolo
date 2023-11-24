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

// I will store the tmpEdgeList
void parse_Delta(graph * G, const char *deltaFileName) {
    free(G->edgeList);
    free(G->edgeListPtrs);
    FILE *file = fopen(deltaFileName, "r");
    if (file == NULL) {
        printf("Cannot open the input file: %s\n",deltaFileName);
        exit(1);
    }

    // read number of vertices and edges, the first line
    long NV=0, NE=0;
    count_graph(deltaFileName, &NV, &NE);
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
        tmpEdgeList[i].weight = 1.0; 
    }
    fclose(file); 

    if ( G->numVertices < NV ) {
        G->com.resize(NV);
        for (long i = G->numVertices; i<NV; i ++) {
            G->com[i] = -1;
        }
    }
    NV = max(NV, G->numVertices);
    for (int i = 0; i < G->tmpEdgeList.size(); i ++ ) {
        printf("%d -> %d\n ", G->tmpEdgeList[i].head, G->tmpEdgeList[i].tail);
    }
    printf("===================\n");
    G->tmpEdgeList.insert(G->tmpEdgeList.end(), tmpEdgeList, tmpEdgeList + NE);
    for (int i = 0; i < G->tmpEdgeList.size(); i ++ ) {
        printf("%d -> %d\n ", G->tmpEdgeList[i].head, G->tmpEdgeList[i].tail);
    }
    NE += G->numEdges;
    printf("[?] NE=%d NV=%d\n", NE, NV);
    // exit(1);
    
    long *edgeListPtr = (long *)  malloc((NV+1) * sizeof(long));
    assert(edgeListPtr != NULL);
    edge *edgeList = (edge *) malloc(2 * NE * sizeof(edge)); //Every edge stored twice
    assert( edgeList != NULL);
    
#pragma omp parallel for
    for (long i=0; i <= NV; i++)
        edgeListPtr[i] = 0; //For first touch purposes
    
#pragma omp parallel for
    for(long i=0; i<NE; i++) {
        __sync_fetch_and_add(&edgeListPtr[G->tmpEdgeList[i].head+1], 1); //Leave 0th position intact
        __sync_fetch_and_add(&edgeListPtr[G->tmpEdgeList[i].tail+1], 1); //Leave 0th position intact
    }
    for (long i=0; i<NV; i++) {
        edgeListPtr[i+1] += edgeListPtr[i]; //Prefix Sum
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
        long head      = G->tmpEdgeList[i].head;
        long tail      = G->tmpEdgeList[i].tail;
        double weight  = G->tmpEdgeList[i].weight;
        
        long Where = edgeListPtr[head] + __sync_fetch_and_add(&added[head], 1);
        edgeList[Where].head = head;
        edgeList[Where].tail = tail;
        edgeList[Where].weight = weight;
        //Add the other way:
        Where = edgeListPtr[tail] + __sync_fetch_and_add(&added[tail], 1);
        edgeList[Where].head = tail;
        edgeList[Where].tail = head;
        edgeList[Where].weight = weight;
    }

    G->sVertices    = NV;
    G->numVertices  = NV;
    G->numEdges     = NE;
    G->edgeListPtrs = edgeListPtr;
    G->edgeList     = edgeList;
    
    free(tmpEdgeList);
    free(added);
    
    
    printf("[!] parse Delta done ! \n");
}
