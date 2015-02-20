#include <iostream>
#include <string>
#include <fstream>
#include <istream>
#include <sstream>
#include <math.h>
#include <algorithm>
#include <mpi/mpi.h>
#include <mpi/mpio.h>

using namespace std;

int Read_File_PGM(string filename, int argc, char *argv[])
{
    int row = 0, col = 0, numrows = 0, numcols = 0;
    int rank,size;
    std::stringstream ss;
    ifstream infile(filename.c_str(), ios::binary);
    ifstream infile2(filename.c_str(), ios::binary);
    ofstream f("new.pgm");
    string inputLine = "";
    string comment = "";

    // count how many line the image is composed
    string line;
    int num_line;
    for (num_line = 0; getline(infile2, line); ++num_line)
        ;

    //create a matrix containing the same number of line of the image
    string img[num_line];


    // First line : version
    getline(infile,inputLine);
    cout << "Version : " << inputLine << endl;

    // Second line : comment + width and height

    /*
    getline(infile,comment);
    cout << comment << endl;
    */
    ss << infile.rdbuf();

	ss >> numrows >> numcols;

	// Third line: max bitsof grey scale
    int max_bits;
    ss >> max_bits;


    cout << "rows: " << numrows << "; cols: " << numcols << endl;
    cout << "max size: " << max_bits ;

    f << "P2" << endl ;



    /*
    if (comment.at(0) == '#'){
    	f << "# Gamma modified by pulver" << endl;
    }*/
    f << numrows << " " << numcols << endl;
    f << "255" << endl;


    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    //Following lines: fill the img matrix
    for(row = 0; row < num_line; ++row){
    	std::getline(ss , inputLine);
    	img[row] = inputLine;
    }


    // Modify the gamma
        for (row = 0; row < num_line; ++row){
        	int lenght = img[row].length();
        	int num, temp = 0;
        	char space = ' ';
        	double gamma = 2.2;

            for (col = 0; col < lenght; ++col){


            	//until you read a number, put it in temp; when you read a space write temp in out and then the space
            		if(img[row].at(col) != space){
            			num = img[row].at(col) -'0' ;
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
            			f << corrected << ' ';
            			//printf(" I am process %d out of %d\n",rank, size);
            			temp = 0;
            			}


            		}
            	}

            	f << endl;
            }


        /*
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
        			//printf(" I am process %d out of %d\n",rank, size);
        			temp = 0;
        			}


        		}
        	}

        	f << endl;
        }


    }*/

infile.close();
MPI_Finalize();

}

int main(int argc,char *argv[]) {
  Read_File_PGM(argv[1],argc, argv);
  return 0;
}
