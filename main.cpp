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
    int row = 0, col = 0;
    int rank;
    int numTask = 0, sendCount, recvCount, source;
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

    //create a matrix containing the same number of line of the image and a matrix to store the value send by other process
    string img[num_line];
    string recvBuf[num_line / 4];
    string partialResult[num_line/4];

    for (int i=0; i < 4; ++i){
    	getline(infile,inputLine);
    	if (i == 1 && inputLine.at(0) == '#'){
    		f << "#Gamma corrected by pulver";
    	}
    	if (i == 3 && inputLine.length() > 4){
    		break;
    	}
    	f << inputLine << endl;
    }



    ss << infile.rdbuf();

    //Following lines: fill the img matrix
       for(row = 0; row < num_line; ++row){
       	std::getline(ss , inputLine);
       	img[row] = inputLine;
       }

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numTask);

    if (numTask == 4 ){
    	int source = 0;
    	int sendCount = 4;
    	int recvCount = 4;

    	MPI_Scatter(img, sendCount,MPI_INT,recvBuf, recvCount,MPI_INT, source,MPI_COMM_WORLD);

    	for (row = 0; row < num_line/4; ++row){
    		int lenght = recvBuf[row].length();
    		int num, temp = 0;
    		char space = ' ';
    		double gamma = 2.2;
    		std::stringstream ss;
    		for (col = 0; col < lenght; ++col){


    			//until you read a number, put it in temp; when you read a space write temp in out and then the space
    			if(recvBuf[row].at(col) != space){
    				num = recvBuf[row].at(col) -'0' ;
    				temp = temp * 10 + num;

    			}else{
    				if(temp == 0) {
    					ss << space;
    				}else {
    					int corrected = (int)(255 * pow(((double)temp/(double)255), gamma));
    					ss << corrected << ' ';
    					temp = 0;
    				}


    			}
    		}
    		getline(ss,partialResult[row]);
    		cout << "rank= " << rank << " : " <<  ss << endl ;
    	}

    }else  {
    	printf("Error, you must specify 4 tasks\n");
    }


    /*
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
            }*/


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
return 0;

}

int main(int argc,char *argv[]) {
  Read_File_PGM(argv[1],argc, argv);
  return 0;
}
