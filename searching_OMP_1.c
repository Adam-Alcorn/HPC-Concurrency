#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h> 
#include <omp.h>
////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////

char *textData;
int textLength;

char *patternData;
int patternLength;

clock_t c0, c1;
time_t t0, t1;

void outOfMemory()
{
	fprintf (stderr, "Out of memory\n");
	exit (0);
}

void readFromFile (FILE *f, char **data, int *length)
{
	int ch;
	int allocatedLength;
	char *result;
	int resultLength = 0;

	allocatedLength = 0;
	result = NULL;

	

	ch = fgetc (f);
	while (ch >= 0)
	{
		resultLength++;
		if (resultLength > allocatedLength)
		{
			allocatedLength += 10000;
			result = (char *) realloc (result, sizeof(char)*allocatedLength);
			if (result == NULL)
				outOfMemory();
		}
		result[resultLength-1] = ch;
		ch = fgetc(f);
	}
	*data = result;
	*length = resultLength;
}

int readData (int testNumber)
{
	FILE *f;
	char fileName[1000];
#ifdef DOS
        sprintf (fileName, "inputs\\test%d\\text.txt", testNumber);
#else
	sprintf (fileName, "inputs/test%d/text.txt", testNumber);
#endif
	f = fopen (fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile (f, &textData, &textLength);
	fclose (f);
#ifdef DOS
        sprintf (fileName, "inputs\\test%d\\pattern.txt", testNumber);
#else
	sprintf (fileName, "inputs/test%d/pattern.txt", testNumber);
#endif
	f = fopen (fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile (f, &patternData, &patternLength);
	fclose (f);

	printf ("Read test number %d\n", testNumber);

	return 1;

}

int hostMatch(long *comparisons) {
	//create vairables and initalise to specified outputs.
		int i,patternElem,dataElem,lastI,pindex;
		i = 0;
		patternElem = 0;
		dataElem =0;
		pindex = -1;
        lastI = textLength-patternLength;
   		(*comparisons) = 0;
	// create myvar to be of type long.
		long myvar;
		myvar = 0;
		bool found = false; 
	
		//initialised variables to firstprivate (meaning they retain their value previously assigned pre entering parallel section)
		//initialised variables to private (meaning they do not retain their value, therefore the value needs updated inside the loop)
		//initialised variables to shared (meaning they are shared amongst threads. In this case pindex is very important as no two threads should be updating this)
		//reduction added to count the number of comparisons. 
        #pragma omp parallel for num_threads(1) firstprivate(textData,patternData,lastI, patternLength) private(patternElem,dataElem,i) shared(pindex,found) schedule(static) reduction(+ : myvar)
        for(i=0; i <= lastI; i++) {
			if(!found){ // flag set to reduce the amount of comparisons
                patternElem=0;
				dataElem=i; //k 
				
                while(patternElem < patternLength && textData[dataElem] == patternData[patternElem]) {
					//printf ("In While");
				// checks wether the element in text data == the element at pattern 
				//increment dataElem and patternElem for next iteration, note an increase in myvar aka comparisons. 
                        patternElem++;
						myvar++;
						dataElem++;
				}

				//if(patternElem > 0){ 
				//printf ("patternElem: %d patternlength: %d\n", patternElem,patternLength);
				//printf ("\n i: %d\n", i);
				//}; 
				

                if (patternElem == patternLength) {

					//printf ("\n i: %d\n", i);
					printf ("In IF \n");
					#pragma omp critical 
					{ // invoke a critical section so no other threads can update this index.      
                        pindex = i;
						printf ("P_INDEX %d\n", pindex);
						found = true; // set the flag to true, resulting in no more comparisons being computed.
					}
            	}else if(patternElem != patternLength)
					myvar++; // if not found increment comparisons. 
				}
		}
	//set comparisons equal to (*comparisons) + myvar. Since myvar was private the update is recorded only for one thread then summed at the end; 
        (*comparisons) = (*comparisons)+ myvar;
        return pindex;
}

void processData()
{
	unsigned int result;
        long comparisons;

	printf ("Text length = %d\n", textLength);
	printf ("Pattern length = %d\n", patternLength);

	result = hostMatch(&comparisons);
	if (result == -1)
		printf ("Pattern not found\n");
	else
		printf ("Pattern found at position %d\n", result);
        printf ("# comparisons = %ld\n", comparisons);

}

int main(int argc, char **argv)
{
	int testNumber;


	testNumber = 1;
	while (readData (testNumber))
	{
		c0 = clock(); t0 = time(NULL);	
   	 	processData();
		c1 = clock(); t1 = time(NULL);
                printf("Test %d elapsed wall clock time = %ld\n", testNumber, (long) (t1 - t0));
                printf("Test %d elapsed CPU time = %f\n\n", testNumber, (float) (c1 - c0)/CLOCKS_PER_SEC); 
		testNumber++;
	}


}
