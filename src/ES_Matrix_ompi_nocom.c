#include "stdio.h"
#include "stdlib.h"
#include <string.h> 
#include <math.h> 
#include "time.h"
#include <sys/time.h> 
#include <unistd.h> 
#include <getopt.h> 
#include "mpi.h"
#include <omp.h>
#include "RandomChange.h"
#include "GSEA.h"
#include "IO.h"

#define ERRM "ES Matrix error:"

char *USAGE =
"\n"
"Usage:"
"  ES_Matrix_ompi_nocom [options]\n"
"\n"
"  general options before command by MPI:\n"
"	 -n process_num : Total number of processes\n"
"	 -ppn pernum: the number of processes in each node\n"
"	 -hostfile hostfile:  list the IP or Hostname of nodes"
"\n"
"  general options:\n"
"    -t --thread: the number of threads in per process_num\n"
"	 -l	--siglen: the length of Gene Expression Signature\n"
"\n"
"  input/output options \n"
"    -1 --input1: a parsed profiles's file from pretreatment stage.\n"
"    -2 --input2: another parsed profiles's file from pretreatment stage.\n"
"	 -o --output: output file ,distributed in every nodes ,with ES Matrix\n";
 
void Usage();
void split_data(int size, int n, int rank, int* begin, int* end, int* local_n); 
void getTriples(int local_P, int genelen, int siglen, int profilenum, int linelen, int begin, int end,  char *file, struct Profile_triple * triples);
void getPartTriples(int genelen, int siglen, int profilenum, int linelen, int begin, int end,  char *file, struct Profile_triple * triples);

int main(int argc,char *argv[])
{	
	int i,j;
	int genelen;
	int profilenum1,profilenum2;
	int linelen1,linelen2;
	struct Profile_triple *triples1,*triples2;
	float **local_ES_Matrix;		//part of the ES_Matrix in this process
	int	my_rank;   /* My process rank           */
    int	p;         /* The number of processes   */
    int source,dest;  
    int tag = 0;
    MPI_Status  status;
	int local_P;	//the data number of each processes must hand
	int begin,end;
	int parameternum;
	int corenum;
	int siglen;

	double start,finish,duration;
	
	/* Let the system do what it needs to start up MPI */
    MPI_Init(&argc, &argv);

    /* Get my process rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    /* Find out how many processes are being used */
    MPI_Comm_size(MPI_COMM_WORLD, &p);
	
	/* check parameter*/
	if(my_rank == 0)
	{
		parameternum = argc;
		if(parameternum == 1)
			Usage();
	}
	MPI_Bcast(&parameternum, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if(parameternum == 1)
	{
		MPI_Finalize();
		exit(0);
	}
	
	// Unset flags (value -1).
	corenum = -1;
	siglen = -1;	
    // Unset options (value 'UNSET').
	char * const UNSET = "unset";
    char * input1   = UNSET;
	char * input2   = UNSET;
	char * output   = UNSET;
	
	int c;
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"thread",             required_argument,        0, 't'},
			{"siglen",             required_argument,        0, 'l'},
			{"input1",             required_argument,        0, '1'},
			{"input2",             required_argument,        0, '2'},
			{"output",             required_argument,        0, 'o'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "t:l:1:2:o:",
            long_options, &option_index);
	
		if(c==-1)	break;
		
		switch (c) {
		
		case 0:
			// A flag was set. //
			break;

		case '1':
			if (input1 == UNSET) 
			{
				input1 = optarg;
			}
			else 
			{
				if(my_rank==0)
				{
					fprintf(stderr, "%s --input1 set more than once\n", ERRM);
					Usage();
				}		
				MPI_Finalize();
				exit(0);
			}
			break;
		
		case '2':
			if (input2 == UNSET) 
			{
				input2 = optarg;
			}
			else 
			{
				if(my_rank==0)
				{
					fprintf(stderr, "%s --input2 set more than once\n", ERRM);
					Usage();
				}		
				MPI_Finalize();
				exit(0);
			}
			break;
		
		case 'o':
			if (output == UNSET) 
			{
				output = optarg;
			}
			else 
			{
				if(my_rank==0)
				{
					fprintf(stderr, "%s --output set more than once\n", ERRM);
					Usage();
				}		
				MPI_Finalize();
				exit(0);
			}
			break;
		
		case 't':
			if (corenum < 0) {
				corenum = atoi(optarg);
				if (corenum < 1) {
					if(my_rank==0)
					{
						fprintf(stderr, "%s --thread must be a positive integer\n", ERRM);
						Usage();
					}		
					MPI_Finalize();
					exit(0);
				}
			}
			else {
				if(my_rank==0)
				{
					fprintf(stderr,"%s --thread set more " "than once\n", ERRM);
					Usage();
				}		
				MPI_Finalize();
				exit(0);
			}
			break;
			
		case 'l':
			if (siglen < 0) {
				siglen = atoi(optarg);
				if (siglen < 1) {
					if(my_rank==0)
					{
						fprintf(stderr, "%s --siglen must be a positive integer\n", ERRM);
						Usage();
					}		
					MPI_Finalize();
					exit(0);
				}
			}
			else {
				if(my_rank==0)
				{
					fprintf(stderr,"%s --siglen set more " "than once\n", ERRM);
					Usage();
				}		
				MPI_Finalize();
				exit(0);
			}
			break;
			
		default:
			// Cannot parse. //
			if(my_rank==0)
				Usage();
			MPI_Finalize();
			exit(0);
		}		
	}

	//check the parameters
	if(corenum == -1)
	{
		if(my_rank==0)
			fprintf(stderr,"Not Set thread parameter!\n");
		MPI_Finalize();
		exit(0);
	}
	if(siglen == -1)
	{
		if(my_rank==0)
			fprintf(stderr,"Not Set siglen parameter!\n");
		MPI_Finalize();
		exit(0);
	}
	
	if(output == UNSET)
	{
		if(my_rank==0)
			fprintf(stderr,"Not Set output parameter!\n");
		MPI_Finalize();
		exit(0);
	}
	
	
	//barrier all processes to compute time
	MPI_Barrier(MPI_COMM_WORLD); 
	if(my_rank == 0){
		printf("Profile Set is Loading...!\n");
		GET_TIME(start);
	}
	
	//read file parameters in all processes
	ReadFilePara(input1, &profilenum1, &genelen, &linelen1);
	ReadFilePara(input2, &profilenum2, &genelen, &linelen2);
	
	//input file check
	if( profilenum1 <= 0 || genelen <= 0)
	{
		if(my_rank==0)
			fprintf(stderr,"this file input1 is not exist!\n");
		MPI_Finalize();
		exit(0);
	}
	
	if( profilenum2 <= 0 || genelen <= 0)
	{
		if(my_rank==0)
			fprintf(stderr,"this file input2 is not exist!\n");
		MPI_Finalize();
		exit(0);
	}

	// compute the local size 、up boundary and down boundary for every process in dataset1
	split_data(profilenum1, p, my_rank, &begin, &end, &local_P);
	
	/*****read the local part file of dataset1 in every process and get their triples****************/
	triples1 = (struct Profile_triple *)malloc(sizeof(struct Profile_triple)*local_P);	
	getTriples(local_P, genelen, siglen, profilenum1, linelen1, begin, end, input1, triples1);
	
	/********************para load profile dataset2 by openmp******************************/
	//allocate the triples memory for dataset2
	triples2 = (struct Profile_triple *)malloc(sizeof(struct Profile_triple)*profilenum2);
	#pragma omp parallel num_threads(corenum)
	{
		int local_t;	//the data number of each thread must hand
		int begin_t,end_t;
		int threadID = omp_get_thread_num();
		
		// compute the local size 、up boundary and down boundary for every thread in dataset2
		split_data(profilenum2, corenum, threadID, &begin_t, &end_t, &local_t);
		
		// compute the begin_t to end_t triples
		getPartTriples(genelen, siglen, profilenum2, linelen2, begin_t, end_t, input2, triples2);		
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	if(my_rank == 0){
		GET_TIME(finish);
		//compute the IO time
		duration = finish-start;     
		printf("loading IO and prework time in no communication way: %.4f s\n",duration);

		printf("Paral compute the ES_Matrix is Starting...!\n");
		GET_TIME(start);
	}
	
	/*
	if(my_rank == 0){
		int k;
		for(k=0;k<siglen;k++)
			printf("%d ",triples1[0].gsUp[k]);
		printf("\n");
		for(k=0;k<genelen;k++)
			printf("%d ",triples2[profilenum2-1].index[k]);
		printf("\n");
	}
	*/
	
	/********************para compute the part of ES_Matrix******************************/
	//allocate the local_ES_Matrix memory
	local_ES_Matrix = (float **)malloc(local_P*sizeof(float *));
	for(i=0;i<local_P;i++)
		local_ES_Matrix[i] = (float *)malloc(profilenum2*sizeof(float));
	#pragma omp parallel num_threads(corenum)
	{
		int k,t;
		int local_t;	//the data number of each thread must hand
		int begin_t,end_t;
		int threadID = omp_get_thread_num();
		
		// compute the local size 、up boundary and down boundary for every thread in dataset2
		split_data(profilenum2, corenum, threadID, &begin_t, &end_t, &local_t);
		
		// compute the part of the ES matrix
		for(k=0;k<local_P;k++)
			for(t=begin_t;t<end_t;t++)
				local_ES_Matrix[k][t] = ES_Profile_triple(triples1[k],triples2[t],genelen,siglen);
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	if(my_rank == 0){
		GET_TIME(finish);
		//compute the compute time
		duration = finish-start;     
		printf("Paral compute the ES_Matrix time: %.4f s\n",duration);
		
		printf("Writing file is Starting...!\n");
		GET_TIME(start);
	}
	
	/*
	if(my_rank == 0){
		int k;
		for(k=0;k<profilenum2;k++)
			printf("%f ",local_ES_Matrix[0][k]);
		printf("\n");
	}
	*/
	
	char Res[128];
	sprintf(Res,"%s_%d.txt",output,my_rank);
	WritetxtResult(0, local_P, profilenum2, Res, local_ES_Matrix);
	
	MPI_Barrier(MPI_COMM_WORLD);
	if(my_rank == 0){
		GET_TIME(finish);
		//compute the write time
		duration = finish-start;     
		printf("Write Result spent: %.4f s\n",duration);
	}
	
	//free the memory
	for(i=0;i<local_P;i++)
		free(local_ES_Matrix[i]);
	free(local_ES_Matrix);
	free(triples1);
	free(triples2);
	
	MPI_Finalize();
	return 0;
}


void split_data(int size, int n, int my_rank, int* begin, int* end, int* local_n)
{
	*local_n = size / n;  
	int leave = size % n;
	if(my_rank < leave){
		(*local_n)++;   
		*begin = my_rank*(*local_n);
	}else{	
		*begin = my_rank*(*local_n) + leave; 
	}	 
	*end = *begin + *local_n;
}

//read the begin to end line part of profile file and get their triples  
void getTriples(int local_P, int genelen, int siglen, int profilenum, int linelen, int begin, int end,  char *file, struct Profile_triple * triples)
{
	int i;
	//allocate the temp memory
	short **profileSet = (short **)malloc(local_P*sizeof(short *));
	for(i=0;i<local_P;i++)
		profileSet[i] = (short *)malloc(genelen*sizeof(short));	
	short **tmp_profiles = (short **)malloc(profilenum*sizeof(short *));
	for(i=0;i<profilenum;i++)
		tmp_profiles[i] = (short *)malloc(genelen*sizeof(short));
	
	//read file and get the proper data
	ReadFile(file, linelen, begin, end, profilenum, genelen, tmp_profiles);
	for(i=0; i<local_P; i++)	
		memcpy(profileSet[i],tmp_profiles[i+begin],genelen*sizeof(short));
	
	//get the triple for every profile	
	for(i=0;i<local_P;i++)
		triples[i] = getTriple(profileSet[i], genelen, siglen);
	
	//free the temp memory
	for(i=0;i<local_P;i++)
		free(profileSet[i]);
	free(profileSet);
	for(i=0;i<profilenum;i++)
		free(tmp_profiles[i]);
	free(tmp_profiles);		
}

//read the begin to end line part of profile file and get their triples in proper part of the  triples vector
void getPartTriples(int genelen, int siglen, int profilenum, int linelen, int begin, int end,  char *file, struct Profile_triple * triples)
{
	int i;
	//allocate the temp memory
	short **profileSet = (short **)malloc(profilenum*sizeof(short *));
	for(i=0;i<profilenum;i++)
		profileSet[i] = (short *)malloc(genelen*sizeof(short));	
	
	//read file and get the proper data
	ReadFile(file, linelen, begin, end, profilenum, genelen, profileSet);
	
	//get the triple for every profile	
	for(i=begin;i<end;i++)
		triples[i] = getTriple(profileSet[i], genelen, siglen);
	
	//free the temp memory
	for(i=0;i<profilenum;i++)
		free(profileSet[i]);
	free(profileSet);		
}

void Usage() {
	fprintf(stderr, "%s\n", USAGE);
}  /* Usage */