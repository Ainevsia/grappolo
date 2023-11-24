#include "input_output.h"
#include "defs.h"
#include "sstream"
#include "utilityStringTokenizer.hpp"


void parse_UndirectedEdgeListWeightedDelta(graph * G, char *fileName) {
    printf("Parsing a SingledEdgeList formatted file as a general graph...\n");
    printf("WARNING: Assumes that the graph is undirected -- an edge is stored ONLY ONCE!.\n");
    int nthreads = 0;
    
#pragma omp parallel
    {
        nthreads = omp_get_num_threads();
    }
    
    double time1, time2;
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        printf("Cannot open the input file: %s\n",fileName);
        exit(1);
    }

    // read number of vertices and edges
    long NV=0, NE=0;
    fscanf(file, "%ld %ld", &NV, &NE);
    
    printf("|V|= %ld, |E|= %ld \n", NV, NE);
    printf("Weights will be processed as-is.\n");
    /*---------------------------------------------------------------------*/
    /* Read edge list: U V W                                             */
    /*---------------------------------------------------------------------*/
    edge *tmpEdgeList = (edge *) malloc( NE * sizeof(edge)); //Every edge stored ONCE
    assert( tmpEdgeList != NULL);
    long Si, Ti;
    double Twt = 1.0;
    time1 = omp_get_wtime();
    for (long i = 0; i < NE; i++) {
#if defined(ALL_WEIGHTS_ONE)
        fscanf(file, "%ld %ld", &Si, &Ti);
#else
        fscanf(file, "%ld %ld %lf", &Si, &Ti, &Twt);
#endif
#if defined(PRINT_PARSED_GRAPH)
        printf("%ld %ld %lf\n", Si, Ti, Twt);
#endif
        assert((Si >= 0)&&(Si < NV));
        assert((Ti >= 0)&&(Ti < NV));
        tmpEdgeList[i].head   = Si;          //The S index
        tmpEdgeList[i].tail   = Ti;          //The T index: Zero-based indexing
#if defined(ALL_WEIGHTS_ONE)
        tmpEdgeList[i].weight = 1.0; //Make it positive and cast to Double?
#else
        tmpEdgeList[i].weight = (double)Twt; //Make it positive and cast to Double?
#endif
    }//End of outer for loop
    fclose(file); //Close the file
    time2 = omp_get_wtime();
    printf("Done reading from file: NE= %ld. Time= %lf\n", NE, time2-time1);
    
    ///////////
    time1 = omp_get_wtime();
    long *edgeListPtr = (long *)  malloc((NV+1) * sizeof(long));
    assert(edgeListPtr != NULL);
    edge *edgeList = (edge *) malloc( NE * sizeof(edge)); //Every edge stored once
    assert( edgeList != NULL);
    time2 = omp_get_wtime();
    printf("Time for allocating memory for storing graph = %lf\n", time2 - time1);
    
#pragma omp parallel for
    for (long i=0; i <= NV; i++)
        edgeListPtr[i] = 0; //For first touch purposes
    
    //////Build the EdgeListPtr Array: Cumulative addition
    time1 = omp_get_wtime();
#pragma omp parallel for
    for(long i=0; i<NE; i++) {
        __sync_fetch_and_add(&edgeListPtr[tmpEdgeList[i].head+1], 1); //Leave 0th position intact
    }
    for (long i=0; i<NV; i++) {
        edgeListPtr[i+1] += edgeListPtr[i]; //Prefix Sum:
    }
    //The last element of Cumulative will hold the total number of characters
    time2 = omp_get_wtime();
    printf("Done cumulative addition for edgeListPtrs:  %9.6lf sec.\n", time2 - time1);
    printf("Sanity Check: |E| = %ld, edgeListPtr[NV]= %ld\n", NE, edgeListPtr[NV]);
    printf("*********** (%ld)\n", NV);
    
    time1 = omp_get_wtime();
    //Keep track of how many edges have been added for a vertex:
    printf("About to allocate for added vector: %ld\n", NV);
    long  *added  = (long *)  malloc( NV  * sizeof(long));
    printf("Done allocating memory fors added vector\n");
    assert( added != NULL);
#pragma omp parallel for
    for (long i = 0; i < NV; i++)
        added[i] = 0;
    
    printf("About to build edgeList...\n");
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
    time2 = omp_get_wtime();
    printf("Time for building edgeList = %lf\n", time2 - time1);
    
    G->sVertices    = NV;
    G->numVertices  = NV;
    G->numEdges     = NE;
    G->edgeListPtrs = edgeListPtr;
    G->edgeList     = edgeList;
    
    free(tmpEdgeList);
    free(added);
    
}//End of parse_DirectedEdgeListWeighted()
