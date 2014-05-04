// Author: ilyearer
// Email: ilyearer@gmail.com
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <ctime>
#include <unistd.h>

using namespace std;

struct AddressNode
{
     int data;
     struct AddressNode *next;
}Header = {0, NULL}, *newAddress, *currentAddress, *previousAddress, *temp;


int IsPowerOfTwo (unsigned int x)
{
  return ((x != 0) && !(x & (x - 1)));
}

// LRU Functions
void swap(int* a, int* b)
{
     int temp;
     temp = *a;
     *a = *b;
     *b = temp;
}
void update_recently_used(int value, int* array, int associativity)
{
     int i, index = 0;
     bool found = false;
     
     for(i = 0; i < associativity - 1; i++)
     {
          if((array[i] == value)&&(!found))
          {
               index = i;
               found = true;
               i--;
          }
          else if(found)
               swap(&array[i], &array[i+1]);
     }
}

int main(int argc, char *argv[])
{
	string input_trace_file; // used for file handling
	char fileRead[80];

	int size = 1024, block_size = 32, associativity = 1; // hold program arguments
	
//    vector<int> address;	// used for address loads
	int current_address = 0, prev = 0;
	int cache_hit = 0, cache_miss = 0, LRU;
	bool found = false;

	int numBlocks, numSets, setLocation, blockLocation = 0, byteOffset;
	int numLoads = 0;
	int ***cache;// used for cache simulation
	int **recently_used;


	// boolean used to loop in case an exception is thrown
	bool retry = false;

	try
	{

		string error;

		bool tflag = false; // Used to indicate that -t option has been passed
		int c;
		int opterr = 0;
		while( (c = getopt(argc, argv, "t:s:b:a:")) != -1)
		{
			switch(c)
			{
				case 't':
					input_trace_file = optarg;
					tflag = true;
					break;

				case 's':
					size = atoi(optarg);
					if(size > 65536)
					{
						error = "Cache size cannot exceed 64KB.";
						throw error;
					}
					else if(size < 1)
					{
						error = "Cache size cannot be less than 1 Byte.";
						throw error;
					}
					else if(!IsPowerOfTwo(size))
					{
						 error = "Cache size must be a power of two.";
						 throw error;
					}
					break;

				case 'b':
					block_size = atoi(optarg);
					if(block_size > 256)
					{
						error = "Block size cannot exceed 256 Bytes.";
						throw error;
					}
					else if(block_size < 1)
					{
						error = "Block size cannot be less than 1 Byte.";
						throw error;
					}
			  		else if(!IsPowerOfTwo(block_size))
					{
						 error = "Block size must be a power of two.";
						 throw error;
					}
					break;

				case 'a':
					associativity = atoi(optarg);
			  		if(!IsPowerOfTwo(associativity))
					{
						 error = "Cache associativity must be a power of two.";
						 throw error;
					}
					break;

				case '?':
					if( (optopt == 't') || (optopt == 's') || (optopt == 'b') || (optopt == 'a') )
					{
						cerr << "Option -" << (char)optopt << " requires an argument.\n";
					}
					else if(isprint(optopt))
					{
						cerr << "Unknown option '-" << (char)optopt << "'.\n";
					}
					else
					{
						fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
					}
					return 1;
				default:
					abort();
			}
		}

		//************************* exception handling logic *********************

		if(!tflag)
		{
			error = "Option '-t' is required.\n";
			throw error;
		}
		else if(associativity > (size / block_size))
		{
			error = "Max associativity exceeded ";// + (size / block_size);
			throw error;
		}

		//************************* exception handling logic *********************
			

		// loads input file, throws exception if it doesn't exist
		ifstream infile(input_trace_file.c_str());
		
		// make sure the file exists
        if(!infile)
        {
             error = "File \"" + input_trace_file + "\" does not exist.";
             throw error;
        }


        //////// Link List Address Loading
        previousAddress = &Header;
        currentAddress = Header.next;
        while(currentAddress != NULL) //moves to end of list
        {
             previousAddress = currentAddress;
             currentAddress = currentAddress->next;
        }
			

		while( (!infile.eof()) && (infile.good()) )
		{
			infile.getline(fileRead, 80);
			
			// Create new node to hold current address read from file
			newAddress = (struct AddressNode *)malloc(sizeof(struct AddressNode));
			newAddress->data = previousAddress->data + atoi(fileRead); // Make sure address is absolute rather than relative
			newAddress->next = NULL;
			
			// insert the newAddress into the end of the list
			// since previous and current point to the last address and null pointer
			previousAddress->next = newAddress;
			newAddress->next = currentAddress;
			
			// move down to last address again so you don't overwrite
			previousAddress = newAddress;
			currentAddress = newAddress->next;
			
			numLoads++;
		}

/**************** Added code ***************************/

		// reset previousAddress/currentAddress to Header
		previousAddress = &Header;
		currentAddress = Header.next;

		string sentinnel;

		// determine set and block dimensions
		numBlocks = size / block_size;
		numSets = numBlocks / associativity;

		int numThreads;
		if(numSets >= 4)
		{
			numThreads = 4;
		}
		else
		{
			numThreads = numSets;
		}

		AddressNode** addressArray = new AddressNode*[numThreads];
		//(AddressNode*) malloc(sizeof(AddressNode)*numThreads);
		for(int i = 0; i < numThreads; i++)
		{
			AddressNode* head = new AddressNode;
			head->data = 0;//i;
			head->next = NULL;
			addressArray[i] = head;
		}

		for(int i = 0; i < numThreads; i++)
		{
			cout << "Thread " << i << ": " << addressArray[i]->data << "\t" << addressArray[i] << "\t" << &addressArray[i] << endl;
		}

		// pre-sim output
		cout << "program_name: " << input_trace_file << endl;
		cout << "cache_size: " << size << endl;
		cout << "block_size: " << block_size << endl;
		cout << "associativity: " << associativity << endl;
		cout << "total_lds: " << numLoads << endl;

		cout << "Number of sets: " << numSets << endl;

		while(currentAddress->next != NULL)
		{
			// collect next address call
			current_address = currentAddress->data;

			// move to next AddressNode for next load
			previousAddress = currentAddress;
			currentAddress = currentAddress->next;

			// determine location to place in cache
			setLocation = (current_address / block_size)%(numSets);
			byteOffset = current_address % block_size;


			cout << "Current Address: " << current_address << endl;
			cout << "\tSet: " << setLocation << "\n\tOffset: " << byteOffset << endl;
			cout << "\tThread: " << setLocation % numThreads << endl;
			cout << "\tThread Set: " << setLocation / 4 << endl;
			getline(cin, sentinnel);

			

		}

/**************** Added code ***************************/
		
/*	
		// determine set and block dimensions
		numBlocks = size / block_size;
		numSets = numBlocks / associativity;
	
		// pre-sim output
		cout << "program_name: " << input_trace_file << endl;
		cout << "cache_size: " << size << endl;
		cout << "block_size: " << block_size << endl;
		cout << "associativity: " << associativity << endl;
		cout << "total_lds: " << numLoads << endl;
	
		// resize the recently_used array to contain the proper # of elements
        recently_used = new int*[numSets];
        for(int i = 0; i < numSets; i++)
        {
            recently_used[i] = new int[associativity];
            for(int j = (associativity - 1); j >= 0; j--)
                 recently_used[i][j] = j;
        }
    
		// resize the 3d array to match cache specs

        cache = new int**[numSets];
		for(int i = 0; i < numSets; i++)
		{
			cache[i] = new int*[associativity];
			for(int j = 0; j < associativity; j++)
				cache[i][j] = new int[block_size];
		}
		

		clock_t start, end;
		start = clock();

        // reset previousAddress/currentAddress to Header
        previousAddress = &Header;
        currentAddress = Header.next;
        
//        int currentLoad = 1;
//        cout << "|";
        
        // Simulate Cache requests
        while(currentAddress->next != NULL)
        {
            // collect next address call
            current_address += currentAddress->data;
            
            // move to next address for following load
            previousAddress = currentAddress;
            currentAddress = currentAddress->next;


			// determine location to place in cache
			setLocation = (current_address / block_size)%(numSets);
			byteOffset = current_address % block_size;

			// check for hit
			found = false;
			for(int j = 0; j < associativity; j++)
			{
				// if address is found, increment hit count and update recently_used array
				if(cache[setLocation][j][byteOffset] == current_address)
				{
					found = true;
					cache_hit++;
					update_recently_used(j, recently_used[setLocation], associativity);
					break;
				}
			}
			// if no cache hit was found
			if(!found)
			{
				// grab the Least Recently Used
				LRU = recently_used[setLocation][0];
				//LRU = recently_used[setLocation].back();

				// increment miss count
				cache_miss++;

				// load block containing requested address
				for(int k = 0; k < block_size; k++)
				{
					cache[setLocation][LRU][k] = (current_address - byteOffset) + k;
				}

				// update Least Recently Used
				update_recently_used(LRU, recently_used[setLocation], associativity);
			}
//			if(!(currentLoad % (numLoads / 40)))
//                cout << "= ";
//            currentLoad++;
		}
		end = clock();
//		cout << "|";

		// Cache Results Output
		cout << "cache_hits: " << cache_hit << endl;
		cout << "cache_misses: " << cache_miss << endl;
		cout << "cache_miss_rate: " << static_cast<float>(cache_miss)/static_cast<float>(numLoads) << endl;

		cout << "Time required for execution: " << (double)(end-start)/CLOCKS_PER_SEC << " seconds." << "\n\n";
*/

	}
	catch(int exception1)
	{  // if input file is invalid
		if( (exception1 && ifstream::failbit) != 0 )
		{
			cout << "File does not exist!" << endl;
		}
	}
	catch(string exception2)
	{   // if a passed argument is invalid
		cout << exception2 << endl;
	}
	
	
	
	currentAddress = Header.next;
	while(currentAddress != NULL)
	{
		temp = currentAddress;
		currentAddress = currentAddress->next;
		free(temp);
	}

    return 0;
}
