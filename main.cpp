#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <algorithm>
#include "mpi.h"

using namespace std;

int Read_File_PGM(string filename, int argc, char *argv[])
{
    int row = 0, col = 0, numrows = 0, numcols = 0, bits;
    int r, c, val;
    int rank,size;
    std::stringstream ss;
    ifstream infile(filename.c_str(), ios::binary);
    ofstream f("new.pgm");
    string inputLine = "";

    // First line : version
    getline(infile,inputLine);

    cout << "Version : " << inputLine << endl;

    // Second line : width and height
    ss << infile.rdbuf();
    ss >> numrows >> numcols;

    int max_bits;
    ss >> max_bits;
    char pixel;
    unsigned int pixel_value[numrows][numcols];
    cout << "rows: " << numrows << "; cols: " << numcols << endl;
    cout << "max size: " << max_bits << endl;

    f << "P2" << endl ;
    f << numrows << " " << numcols << endl;
    f << "255" << endl;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Following lines : data
    for (row = 0; row < numrows; ++row){


    	std::string streamrow;
        for (col = 0; col < numcols; ++col){


        	//get text from stream and put into a string
        	std::getline(ss , streamrow);
        	int temp = 0, num;
        	char space = ' ';
        	double gamma = 2.2;

        	//until you read a number, put it in temp; when you read a space write temp in out and then the space
        	for (int i = 0; i < streamrow.size(); i++){
        		if(streamrow.at(i) != space){

        			num = streamrow.at(i)-'0' ;
        			temp = temp * 10 + num;
        		}else{
        			if(temp == 0) {
        				f << space;
        			}else {
        			//temp = temp % 255;
        			//temp = temp/255;
        			//temp = pow(temp, gamma);
        			//temp =(int) 255 * temp;
        			//temp = temp % (double) 255;
        			int corrected = (int)(255 * pow(((double)temp/(double)255), gamma));
        			//f << temp << space;
        			f << corrected << space;
        			printf(" I am process %d out of %d\n",rank, size);
        			temp = 0;
        			}


        		}
        	}


			/*
        	//copy every string into name stream and then into the new image file
        	std::string name;
        	std::getline(ss, name);
        	f << name ;
			*/


        }
        f << endl;

    }

infile.close();
MPI_Finalize();
}

int main(int argc,char *argv[]) {
  Read_File_PGM("columns.pgm",argc, argv);
  return 0;
}
