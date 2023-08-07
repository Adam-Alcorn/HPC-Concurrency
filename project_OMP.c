#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h> 
#include <omp.h>

////////////////////////////////////////////////////////////////////////////////
// Program Porject_OMP
// Developed By: Adam Alcorn
////////////////////////////////////////////////////////////////////////////////

// Text data array
char *textData;
// Value for text length
int textLength;

//Control data array
char *controlData;
//Value for control data file length
int controlFileLength;

//Control data array
char *patternData;
//Value for pattern data file length
int patternLength;

// locations of patterns, no greater than 100000, if more space is needed then increment the locOfPatterns size
int locOfPatterns[100000];//text length / pattern length

// Assigning value to blank text - used when accessing the value of the control file
char value = ' ';

// Arbitrary counter
int counter = 0;

// Match code initalisation
int multOrSingleSearchNum = -1;
// Text file num initalisation
int textFileNum = -1;
// Pattern File num initalisation
int patternFileNum = -1;
// Second number initalisation - used when double digit is present
int secondaryNumber = -1;
// Record counter - used to record which value has a digit after it for example if the pattern number is 13 the secondary number is 3
//therefore recordcounter links the pattern number, which corresponds to 2. Corresponding codes: (Match code = 0, text number = 1 and pattern number = 2)
int recordCounter = -1;

// The number of files;
int numberOfFiles = 0;

// result of the concatenation.
int concat = 0;

/*!
 *Method that throws an out of memory error and exits with code 0
 *Developed as part of the inital project package
 */

void outOfMemory()
{
	fprintf (stderr, "Out of memory\n");
	exit (0);
}

/*!
 * Method to read from the text and patern file
 * @param f               Classified as the file
 * @param data        The data which has been read from the file
 * @param length   The overall length of the file being read
 *Developed as part of the inital project package
 */

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

/*!
 *Method to go to and choose the text and pattern files. This method also calls the readFromFile method
 *@param testNumber         The number of the test file to read in.
 *@param patternNumber  The number of the pattern file to read in.
 *Developed as part of the inital project package
 */
int readData (int testNumber, int patternNumber)
{
	FILE *f;
	char fileName[1000];
#ifdef DOS
        sprintf (fileName, "inputs\\text%d.txt", testNumber);
#else
	sprintf (fileName, "inputs/text%d.txt", testNumber);
#endif
	f = fopen (fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile (f, &textData, &textLength);
	fclose (f);
#ifdef DOS
        sprintf (fileName, "inputs\\pattern%d.txt", patternNumber);
#else
	sprintf (fileName, "inputs/pattern%d.txt", patternNumber);
#endif
	f = fopen (fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile (f, &patternData, &patternLength);
	fclose (f);

	printf ("Read test number %d\n", testNumber);

	return 1;

}

/*!
 @abstract Method to count the number of lines in the control file
 */


int countNumberOfFiles() {
    FILE * controlFile;
    char fileName[1000];
    char value;
    
#ifdef DOS
    sprintf(fileName, "inputs\control.txt");
#else
    sprintf(fileName, "inputs/control.txt");
#endif
    //open the control file in read mode
    controlFile = fopen(fileName, "r");
    //check for null
    if (controlFile == NULL)
        return 0;
    // for the value in the control file until the end of the file, count how many lines exist
    for (value = getc(controlFile); value != EOF; value = getc(controlFile))
        // if the line is a new line the add it to the number of files
        if (value == '\n')
            numberOfFiles++;
    fclose(controlFile);
    
}

/*!
 @abstract Method to match the data between the pattern and the text file
 @param comparisons      The number of comparisons made by when searching for a pattern.
 @param matchCode           The matchCode, this will tell the program is it needs to find one or all occurences
 */


int hostMatch(long *comparisons, int matchCode) {

	//create vairables and initalise to specified outputs.
		int i,patternElem,dataElem,lastI,pindex,patternFoundIndex;
		i = 0;
		patternElem = 0;
		dataElem =0;
		pindex = -1;
		patternFoundIndex=0;
        lastI = textLength-patternLength;
   		(*comparisons) = 0;
	// create myvar to be of type long.
		long myvar;
		myvar = 0;
		bool found = false; 
	
		//initialised variables to firstprivate (meaning they retain their value previously assigned pre entering parallel section)
		//initialised variables to private (meaning they do not retain their value, therefore the value needs updated inside the loop)
		//initialised variables to shared (meaning they are shared amongst threads. In this case pindex is very important as no two threads should be updating this)
		//reduction added to count the number of comparisons. Static used as when tested it returned the best speedup, I kept this for debugging purposes as
            // it allows me to see if the comparions, whilst not exact, are reasonable
        #pragma omp parallel for num_threads(4) firstprivate(textData,patternData,lastI, patternLength) private(patternElem,dataElem,i) shared(locOfPatterns,patternFoundIndex) schedule(static) reduction(+ : myvar)
        for(i=0; i <= lastI; i++) {
			if(!found){ // flag set to reduce the amount of comparisons
                patternElem=0;
				dataElem=i; //k 
				
                while(patternElem < patternLength && textData[dataElem] == patternData[patternElem]) {
				// checks wether the element in text data == the element at pattern 
				//increment dataElem and patternElem for next iteration, note an increase in myvar aka comparisons. 
                        patternElem++;
						myvar++;
						dataElem++;
				}
                if (patternElem == patternLength) {
					#pragma omp critical 
					{ // invoke a critical section so no other threads can update this index.
                        if(matchCode == 0)
						found = true; // set the flag to true, resulting in no more comparisons being computed.
						locOfPatterns[patternFoundIndex]= i;
						patternFoundIndex++;
						
					}
            	}else if(patternElem != patternLength){
					myvar++; // if not found increment temp comparisons.
				}
		}
        }
	//set comparisons equal to (*comparisons) + myvar. Since myvar was private the update is recorded only for one thread then summed at the end; 
        (*comparisons) = (*comparisons)+ myvar;
        return pindex;
}

/*!
 @abstract Method to write the result to the file
 @param textFileNum      The number of the text file
 @param patternFileNum   The number of the pattern file
 @param location         The location or response code of the value
 */
void writeDataToFile(int textFileNum, int patternFileNum, int location) {
    FILE * outputFile;
    char fileName[1000];
#ifdef DOS
    //file to write to: result_OMP
    sprintf(fileName, "result_OMP.txt");
#else
    sprintf(fileName, "result_OMP.txt");
#endif
    outputFile = fopen(fileName, "a");
    //null check on the output file
    if (outputFile == NULL) {
        printf("Error!");
        exit(1);
    }
    //write the textFileNum, patternFileNum, Location to the output file.
    fprintf(outputFile, "\n%d %d %d", textFileNum, patternFileNum, location);
    fclose(outputFile);
}



void processData(int textFileNum, int patternFileNum, int matchCode)
{
	unsigned int result;
    long comparisons;
    bool found = false;

	printf ("Text length = %d\n", textLength);
	printf ("Pattern length = %d\n", patternLength);
    //call the host match method to computer the comparisons
	result = hostMatch(&comparisons, matchCode);
    //for the full size of locOfPatterns
	for (size_t p = 0; p < sizeof locOfPatterns/4; p++)
	{
        // if the matchcode is equal to 1 then write the data to the file.
        if (matchCode == 1){
        if(locOfPatterns[p] != -1){
            writeDataToFile(textFileNum,patternFileNum,locOfPatterns[p]);
        }
        }else if(matchCode == 0) {
            //else if the match code = 0 and any value in locOfPatterns is not equal to -1 then return found as true.
            if(locOfPatterns[p] != -1){
                found = true;
            }
            // if the value of found is false and we have reached our final value of locofpatterns then write -1 to the file.
            if ( p == (sizeof locOfPatterns/4)-1 && found == false){
                writeDataToFile(textFileNum,patternFileNum,-1);
            }
            // if the value of found is true and we have reached our final value of locofpatterns then write -2 to the file.
            if ( p == (sizeof locOfPatterns/4)-1 && found == true){
                writeDataToFile(textFileNum,patternFileNum,-2);
            }
	}
    }
        printf ("# comparisons = %ld\n", comparisons);

}

/*!
 @abstract Method to read the control file
 */

int readControlFile() {
    FILE * controlFile;
    char fileName[1000];
#ifdef DOS
    sprintf(fileName, "inputs\control.txt");
#else
    sprintf(fileName, "inputs/control.txt");
#endif
    // Open in read mode
    controlFile = fopen(fileName, "r");
    // check for null file
    if (controlFile == NULL)
        return 0;
    // read the data from the file
    readFromFile(controlFile, & controlData, & controlFileLength);
    fclose(controlFile);
    
    printf("\nControl Data ijk: %s\ns", controlData);
    
}



/*!
 @abstract Method to concat the two values from decipherTests. Both numbers are also strings.
 @param a    Number A
 @param b    Number B
 */

void concat1(int a, int b) {
    
    char s1[20];
    char s2[20];
    
    // Convert both the integers to string
    sprintf(s1, "%d", a);
    sprintf(s2, "%d", b);
    
    // Concatenate both strings
    strcat(s1, s2);
    
    // Convert the concatenated string to integer
    concat = atoi(s1);
}


/*!
 @abstract Method takes the control file and extracts the match code, file number and pattern number.
 @param startpos The start position of the file. Since the numbers are in rows of 7 we mutliple by 7.
 */

int patternCounter = 0;
int textCounter = 0;

void decipherTests(int startpos){
    startpos = (startpos * 7) + textCounter+ patternCounter;
    //Set the values to the same as described at the begining. This method could be called many times so its important to reset these.
    counter = 0;
    multOrSingleSearchNum = -1;
    textFileNum = -1;
    patternFileNum = -1;
    secondaryNumber = -1;
    value = ' ';
    
    //For in i until it reaches a new line character
    for (int i = startpos; value != '\n'; i++) {
        //Set value to vaue within the control file
        value = controlData[i];
        //if the value is equal to a space then skip else do the following:
        if (value != ' ') {
            // if the counter == 0, refering to the first number then set it to the charater minus the char for 0. This returns the int value.
            if (counter == 0) {
                multOrSingleSearchNum = value - '0';
            // if the counter == 1, refering to the first number then set it to the charater minus the char for 1. This returns the int value.
            } else if (counter == 1) {
                textFileNum = value - '0';
            // if the counter == 2, refering to the first number then set it to the charater minus the char for 2. This returns the int value.
            } else if (counter == 2) {
                patternFileNum = value - '0';
            }
            // Incase we are dealing with 2 digit numbers the following must be used:
            // If the control data value at i+1 is also a character that is not a space and not a return
            if (controlData[i + 1] != '\r')
                if (controlData[i + 1] != ' ') {
                    //record the secondary number
                    secondaryNumber = controlData[i + 1] - '0';
                    recordCounter = counter;
                        //if the number which was a secondary number relates to the first character then concate the two values.
                        //Note: the offset needs to be incremented as we have advanced one extra character and the start position is no longer a mutliple of 7
                    if (recordCounter == 0) {
                        concat1(multOrSingleSearchNum, secondaryNumber);
                        multOrSingleSearchNum = concat;
                        //if the number which was a secondary number relates to the second character then concate the two values.
                        //Note: the offset needs to be incremented as we have advanced one extra character and the start position is no longer a mutliple of 7
                    } else if (recordCounter == 1) {
                        concat1(textFileNum, secondaryNumber);
                        textFileNum = concat;
                        textCounter = 1;
                        //if the number which was a secondary number relates to the third character then concate the two values.
                        //Note: the offset needs to be incremented as we have advanced one extra character and the start position is no longer a mutliple of 7
                    } else if (recordCounter == 2) {
                        concat1(patternFileNum, secondaryNumber);
                        patternFileNum = concat;
                        patternCounter = 1;
                    }
                }
            // Check for double digits, if below 9 then reset the offsets to 0
            if(patternFileNum  < 9){
                patternCounter = 0;
            } else if( textFileNum < 9){
                textCounter = 0;
            }
            
            counter++;
        }
    }
    printf("\n multOrSingleSearchNum: %d \n textFileNum: %d \n patternFileNum: %d \n ", multOrSingleSearchNum, textFileNum, patternFileNum);
}

/*!
 @abstract Main method is the main controller and does many things:
 *  Counts the number of files.
 *  Reads the Control file.
 *  Sets the memory of locOfPatterns to -1.
 *  Generates the text file num and pattern file num to be read.
 *  Processes the data leading to the program outputting into the output file.
 */

int main(int argc, char **argv)
{
    // Count the number of lines in the file
    countNumberOfFiles();
    // Read in the control file
    readControlFile();
    // for the number of lines in the file
    for (int z = 0; z <= numberOfFiles; z++){
        // set the memory of locOfPatterns to -1
        memset(locOfPatterns, -1,sizeof locOfPatterns);
        // read the line (z) from the array this means returning a pattern and text file number along with a match code.
        decipherTests(z);
        // read the data such as the pattern and text
        readData (textFileNum, patternFileNum);
        // process the read data with the corresponding textFileNum, patternFileNum, multOrSingleSearchNum.
   	 	processData(textFileNum, patternFileNum, multOrSingleSearchNum);
	}


}

