#include <stdlib.h>

#include <stdio.h>

#include <string.h>

#include <math.h>

#include <time.h>

#include <stdbool.h>

#include <mpi.h>

////////////////////////////////////////////////////////////////////////////////
///Program Searching MPI
///Developed By Adam Alcorn
///Updated: 07/12/2021
///Running the pattern files in parallel against a series of files
///Note: Remeber to create result_MPI.txt
////////////////////////////////////////////////////////////////////////////////

char * textData;
int textLength;

char * patternData;
int patternLength;

char * controlData;
int controlFileLength;

//New Arrays for portion have been created
char * portion;
int portionSize = 0;
//Result is now a global variable due to the requirement within the reduce section in main method
unsigned int result;

clock_t c0, c1;
time_t t0, t1;
//Master process assigned the value of zero
#define MASTER 0
//Number of files used in the whole process
#define NUM_OF_FILES 2

#define SIZES 1000


char value = ' ';

//simple counter
int counter = 0;
//the match code value also the first number in the file
int multOrSingleSearchNum = -1; 
// the text file number also the second number in control file
int textFileNum = -1; 
// pattern file number also the third number in file
int patternFileNum = -1;
//The number used if a second number is present example: any number above 9 (10,11,12,...)
int secondaryNumber = -1; 

int recordCounter = -1;

//The value used to count the offset of the next set of characters in the control file
int nextoffset = 0;
//The result buffer for the gather method
int * rbuf;

//value for the concat result
int concat = 0;

//takes note of the number of files in the text file
int numberOfFiles = 0;

/*!
 *Method that throws an out of memory error and exits with code 0
 *Developed as part of the inital project package
 */

void outOfMemory() {
    fprintf(stderr, "Out of memory\n");
    exit(0);
}

/*!
 * Method to read from the text and patern file
 * @param f               Classified as the file
 * @param data        The data which has been read from the file
 * @param length   The overall length of the file being read
 *Developed as part of the inital project package
 */

void readFromFile(FILE * f, char ** data, int * length) {
    int ch;
    int allocatedLength;
    char * result;
    int resultLength = 0;
    
    allocatedLength = 0;
    result = NULL;
    
    ch = fgetc(f);
    while (ch >= 0) {
        resultLength++;
        if (resultLength > allocatedLength) {
            allocatedLength += 10000;
            result = (char * ) realloc(result, sizeof(char) * allocatedLength);
            if (result == NULL)
                outOfMemory();
        }
        result[resultLength - 1] = ch;
        ch = fgetc(f);
    }
    * data = result;
    * length = resultLength;
}


/*!
 *Method to go to and choose the text and pattern files. This method also calls the readFromFile method
 *@param testNumber         The number of the test file to read in.
 *@param patternNumber  The number of the pattern file to read in.
 *Developed as part of the inital project package
 */
int readData(int testNumber, int patternNumber) {
    FILE * f;
    char fileName[1000];
#ifdef DOS
    sprintf(fileName, "inputs\\text%d.txt", testNumber);
#else
    sprintf(fileName, "inputs/text%d.txt", testNumber);
#endif
    f = fopen(fileName, "r");
    if (f == NULL)
        return 0;
    readFromFile(f, & textData, & textLength);
    fclose(f);
#ifdef DOS
    sprintf(fileName, "inputs\\pattern%d.txt", patternNumber);
#else
    sprintf(fileName, "inputs/pattern%d.txt", patternNumber);
#endif
    f = fopen(fileName, "r");
    if (f == NULL)
        return 0;
    readFromFile(f, & patternData, & patternLength);
    fclose(f);
    
    printf("Read test number %d\n", testNumber);
    
    return 1;
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
    sprintf(fileName, "result_MPI.txt");
#else
    sprintf(fileName, "result_MPI.txt");
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

/*!
 @abstract Method to match the data between the pattern and the text file
 @param rank             The rank of the process currently running, added for mostly debugging reasons
 @param matchCode  The matchCode, this will tell the program is it needs to find one or all occurences
 */

int * locofpatterns;
void hostMatch( int rank, int matchCode) {

    bool found = false;
    bool foundMult = false;
    int i, j, k, lastI;
    int patternFoundIndex = 0;
    int patternElem = 0;
    i = 0;
    j = 0;
    k = 0;
    // the last occurance that the pattern can be found.
    lastI = portionSize - patternLength;
    //boolean value check for found - only works in the case of matchcode = 0;
    //Mainly for speedup purposes
    if (!found) {
        while (i <= lastI && j < textLength) {
            // if the value in the text is equal to the pattern data
            if (portion[k] == patternData[j]) {
                k++;
                j++;
            } else {
                i++;
                k = i;
                j = 0;
            }
            // if the value of j is equal to the pattern length
            if (j == patternLength) {
                if (matchCode == 0) {
                    // set the flag to true, resulting in no more comparisons being done.
                    found = true;
                    //store the value of -2 inside the locofpatterns array
                    locofpatterns[patternElem] = -2;
                } else {
                    // set the boolean which checks if multiple were found to true.
                    foundMult = true;
                    //store the value of i in locofpatterns.
                    locofpatterns[patternElem] = i;
                    patternElem++;
                }
            }
        }
    }
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

void decipherTests(int startpos) {
    // Next offset is a value which is used when dealing with non single digit numbers
    startpos = (startpos * 7) + textCounter + patternCounter;
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
                        //Note: the offset needs to incremented as we have advanced one extra character and the start position is no longer a mutliple of 7
                    if (recordCounter == 0) {
                        concat1(multOrSingleSearchNum, secondaryNumber);
                        multOrSingleSearchNum = concat;
                        //if the number which was a secondary number relates to the second character then concate the two values.
                        //Note: the offset needs to incremented as we have advanced one extra character and the start position is no longer a mutliple of 7
                    } else if (recordCounter == 1) {
                        concat1(textFileNum, secondaryNumber);
                        textFileNum = concat;
                        textCounter = 1;
                        //if the number which was a secondary number relates to the third character then concate the two values.
                        //Note: the offset needs to incremented as we have advanced one extra character and the start position is no longer a mutliple of 7
                    } else if (recordCounter == 2) {
                        concat1(patternFileNum, secondaryNumber);
                        patternFileNum = concat;
                        nextoffset++;
                        textCounter = 1;
                    }
                }
            
            // Check for double digits, if below 9 then reset to 0
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
    controlFile = fopen(fileName, "r");
    if (controlFile == NULL)
        return 0;
    readFromFile(controlFile, & controlData, & controlFileLength);
    fclose(controlFile);
    
    printf("\nControl Data ijk: %s\ns", controlData);
    
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
 @abstract Main Method this is where many different operations happen, these are as follows:
 * Processes read the control file
 * Process 0 assigns the portion of data
 * Process 0 calculates a portion size , start position, and end position
 * Sends data to worker processes
 * Data is recieved from worker processes
 * The host match is computed
 * The results are gathered back into process 0
 * The meth to write the output files is given the expected repsonses
 */

int main(int argc, char ** argv) {
    // Initialize the MPI Section
    MPI_Init(NULL, NULL);
    // Rank of each process
    int rank;
    // Total nuber of processes
    int total_proc;
    //Starting Position of the portion
    int starting_pos = 0;
    //End Position of the portion
    int end_pos = 0;
    //Initalise CommSize to the return the total number of processes
    MPI_Comm_size(MPI_COMM_WORLD, & total_proc);
    //Initalise CommRank to initalise a local variables with the rank of each process
    MPI_Comm_rank(MPI_COMM_WORLD, & rank);
    //Initalise pattern length to 0
    patternLength = 0;
    //count the number of lines in the control file.
    countNumberOfFiles();
    
    //initalise the location of the patterns to size portion size.
    //This means that if every character in the text was the pattern,
    //then we would be able to store every occurance.

    
    printf("\nNumber of Files/Lines: %d", numberOfFiles);
    // For the number of files in the process, begining at 1
    for (int i = 0; i < numberOfFiles; i++) {
        // if the rank is equal to master (0) then do the following
        if (rank == MASTER) {
            //Read in the control file
            readControlFile();
            //Decipher the tests and pattern files
            // i can be updated to run for any of the tests, if you want to run a tests with double digits numbers the offset will have to be set.
            decipherTests(i);
            //Process 0 reads the text data gaining knowledge of the text and pattern data.
            //Array sizes such as pattern length and text length are also generated.
            readData(textFileNum, patternFileNum);
        }
        
        // let the other processes ask for the pattern length and text length.
        MPI_Bcast( & textLength, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
        MPI_Bcast( & patternLength, 1, MPI_INT, MASTER, MPI_COMM_WORLD);

        // if the pattern is less than the lenth of the text then discount the pattern
        if (patternLength <= textLength) {
            // if the process rank is equal to 0
            if (rank == MASTER) {
                counter = 0;
                //Computation for the size of the portion
                //Assign memory for the portion size
                // portion size is a follows: the whole text length divided by the total number of process.
                // To check for overflow between two areas an overlap of pattern length is created
            
                double floatingportion = (textLength / total_proc);
                int portionSize = (int) floatingportion + patternLength;
                // intialise the portion to be of size: portionSize
                portion = (char * ) malloc(portionSize * sizeof(char));
                // for the total processes apart from 0
                for (int j = 1; j < total_proc; j++) {
                    counter = 0;
                    // starting position is the portion size * the process number.
                    starting_pos = portionSize * j;
                    
                    
                    //An if statment to work out if the j being modelled is the final process:
                    //If present then no overlap of pattern length will occur
                    //If j != the totalprocs-1 then the pattern length is added to the end.
                    if (j == total_proc - 1) {
                        end_pos = textLength;
                    } else {
                        end_pos = (starting_pos + portionSize);
                    }
                    //Check: if the end_pos is greater than the text length, then set to the text length.
                    if(end_pos > textLength){
                        end_pos = textLength;
                    }
                    
                    if(starting_pos > textLength){
                        starting_pos = end_pos - portionSize;
                    }
                    //Initalisation of array portion; values range from start positon to end position
    
                    for (int x = starting_pos; x < end_pos; x++) {
                        char a = textData[x];
                        portion[counter] = a;
                        counter++;
                    }
                    
                    //MPI Send pattern Data; to process j; with tag 0; of size pattern length; with type CHAR
                    MPI_Send(patternData, patternLength, MPI_CHAR, j, 0, MPI_COMM_WORLD);
                    
                    //MPI Send portion Size; to process j; with tag 0; of size 1; with type INT
                    MPI_Send( & portionSize, 1, MPI_INT, j, 0, MPI_COMM_WORLD);
                    
                    //MPI Send portion Data; to process j; with tag 0; of size portion size; with type CHAR
                    MPI_Send(portion, portionSize, MPI_CHAR, j, 0, MPI_COMM_WORLD);
                }
                //Assigning the portion to process 0
                counter = 0;
                //Assign memory for the portion size
                //For all the values between 0 and (portion size + pattern length) assign them into te portion array.
                int zeroSize  = 0 + portionSize;
    
                for (int x = 0; x < zeroSize; x++) {
                    portion[counter] = textData[x];
                    counter++;
                }
            } else {
                
                //Assign memory for the pattern size
                patternData = (char * ) malloc(patternLength * sizeof(char));
                // MPI Recieve pattern Data; from process 0; with tag 0; of size pattern length; with type CHAR
                MPI_Recv(patternData, patternLength, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                //MPI Recieve portion Size; from process 0; with tag 0; of size 1; with type INT
                MPI_Recv( & portionSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                //Assign memory for the portion size
                portion = (char * ) malloc(portionSize * sizeof(char));
                //MPI Recieve portion Data; from process 0; with tag 0; of size portion size; with type CHAR
                MPI_Recv(portion, portionSize, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
            }
            //Broadcast the following values:
                //multOrSingleSearchNum
                //textFileNum
                //patternFileNum
            // Tested sending these values against broadcasting and gained a very slight speedup from boradcasting
            MPI_Bcast( & multOrSingleSearchNum, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
            MPI_Bcast( & textFileNum, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
            MPI_Bcast( & patternFileNum, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
            

            locofpatterns = malloc(portionSize * sizeof(int));
            //Set the whole array to -1
            
            for(int i = 0; i < sizeof locofpatterns; i++){
                locofpatterns[i] = -1;
            }
            //Check: if textlength is greater than pattern legnth then the following works.
                // run the host match to check the values of the portion against the values of the text
                hostMatch(rank, multOrSingleSearchNum);
                int recdatasize;
                //initalise the array rbuf to be four times the size of locofpatterns.
                recdatasize = sizeof locofpatterns * total_proc;
                if (rank == MASTER) {
                    rbuf = (int * ) malloc(recdatasize * sizeof(int));
                }
    
                // calculate the size of locofpatterns.
                int locofpatternssize = sizeof locofpatterns;
                //MPI gather the values together
                MPI_Barrier(MPI_COMM_WORLD);
                MPI_Gather(locofpatterns, locofpatternssize, MPI_INT, rbuf, locofpatternssize, MPI_INT, MASTER, MPI_COMM_WORLD);
                //create a bool for found multiple and found single
                bool foundMult = false;
                bool foundSingle = false;
                // if the rank is equal to master to the following
                if (rank == MASTER) {
                    // for all of the gathered values
                    for (int l = 0; l < recdatasize; l++) {
                        // if the match code was 1
                        if (multOrSingleSearchNum == 1) {
                            // check values over -3
                            if (rbuf[l] > -3 && rbuf[l] != -1) {
                                // if a value is found, write that value to the control file
                                writeDataToFile(textFileNum, patternFileNum, rbuf[l]);
                                foundSingle = true;
                             }
                            // if the match code was 0
                        } else if (multOrSingleSearchNum == 0) {
                            // if a value is found, set foundMult to true;
                            if (rbuf[l] == -2)
                                foundMult = true;
                        }
                    }
                    
                    if (foundMult == true && multOrSingleSearchNum == 0) {
                        // if a value is found, write -2 to the control file
                        writeDataToFile(textFileNum, patternFileNum, -2);
                    } else if (foundMult == false && multOrSingleSearchNum == 0) {
                        // if a value is not found for the single searches, write -1 to the control file
                        writeDataToFile(textFileNum, patternFileNum, -1);
                    }
                    if (foundSingle == false && multOrSingleSearchNum == 1) {
                        // if a value is not found for the mutli searches, write -1 to the control file
                        writeDataToFile(textFileNum, patternFileNum, -1);
                    }
                }
            //result if the pattern was larger than the file size
        } else if (rank == 0) {
            writeDataToFile(textFileNum, patternFileNum, -1);
        }
        
    
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    //Free the arrays Since only the master process allocated memory for rbuf and text data it is the only process that needs to individually free these.
    if (rank == 0) {
        free(rbuf);
        free(textData);
    }
    free(patternData);
    free(portion);
    free(locofpatterns);
    MPI_Barrier(MPI_COMM_WORLD);
    //MPI Finialize - signifies the end of parallel section
    MPI_Finalize();
}
