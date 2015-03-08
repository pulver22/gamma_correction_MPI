#include <mpi.h>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <iterator>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <cmath>
#include <climits>
#include <unistd.h>

using namespace std;

typedef struct {
	int width;
	int height;
	int max;
	std::vector<int> data;
} image;

static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

/*
 * Returns how long a string representing the number i would be 
 */
static inline int slength(int i) {
	int counter = 1;
	for(; i>9; i /= 10, ++counter);
	return counter;
}

void read_image(ifstream& in, image& img);
void write_image(ofstream& out, image& img);
void gamma_correct(image& img, float gamma, int rank, int size);
void correct_vector(std::vector<int>& data, float gamma, int old_max, int new_max, int start, int end);
void receive_compute_send(int rank);
void split_send_receive(image& img, float gamma, int old_max, int size);

int main (int argc, char *argv[]) {
	MPI_Init(&argc, &argv);
	int size, rank;
	int name_len;
    int pid;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	// Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];  
    MPI_Get_processor_name(processor_name, &name_len);

	if(rank == 0) {
		if(argc <3) {
			cout << "Expected a file path and a gamma as argument, in this specific order." << endl;
			return EXIT_FAILURE;
		}
		float gamma;
		gamma = atof(argv[2]);
		if(gamma <0) {
			cout << "Gamma need to be non negative. Got " << gamma << endl;
		}

		image img;

		ifstream infile;
		infile.open(argv[1]);
		read_image(infile, img);
		infile.close();

		gamma_correct(img, gamma, rank, size);

		ofstream outfile;
		outfile.open("result.pgm");
		write_image(outfile, img);
		outfile.close();
		// Print off a hello world message
	    printf("PID %d@Processor %s, rank %d"
	    " out of %d processors has finished the job\n",
	    pid, processor_name, rank, size);
	} else {
		receive_compute_send(rank);
	}
	MPI_Finalize();

	return 0;
}

void read_image(ifstream& in, image& img) {
	stringstream ss;
	string inputLine;
	// First line : version
	getline(in,inputLine);
	rtrim(inputLine);

	if(inputLine.compare("P2") != 0) {
		cerr << "Version error. Read '" << inputLine << "' instead." << endl;
		exit(EXIT_FAILURE);
	} else {
		//cout << "Version : " << inputLine << endl;
	}
	// Second line : comment. May not be there.
	char comment_char = in.get();
	if(comment_char == '#') {
		getline(in,inputLine);
	} else {
		//cout << "No comment in the header" << endl;
		ss << comment_char;
	}

	// Continue with a stringstream
	ss << in.rdbuf();
	// Third line : size
	ss >> img.width;
	ss >> img.height;
	ss >> img.max;

	int val;
	for(int row = 0; row < img.height; ++row) {
		for (int col = 0; col < img.width; ++col) {
			ss >> val;
			img.data.push_back(val);
		}
	}
}

void write_image(ofstream& out, image& img) {
	out << "P2" << endl << "# image gamma corrected" << endl;
	out << img.width << " " << img.height << endl << img.max << endl;
	
	int line_length = 0;
	for(int row = 0; row < img.width; ++row) {
		for (int col = 0; col < img.height; ++col) {
			int pixel = img.data[row*img.height + col];
			out << pixel << " ";
			line_length += slength(pixel) + 1;
			// the specs say not to go over 70 chars per line
			if(line_length > 65) {
				out << endl;
				line_length = 0;
			}
		}
	}
}

void gamma_correct(image& img, float gamma, int rank, int size) {
	int old_max = img.max;
	if(size > 1) {
		if(rank == 0) {
			//The master takes care to update the max value of the image
			img.max = (int) min( USHRT_MAX, img.max);
			cout << "New max " << img.max << endl;
			split_send_receive(img, gamma, old_max, size);
		} else {
			receive_compute_send(rank);
		}
	} else {
		// Only one process in the world. Just compute it not in parallel
		img.max = (int) min( USHRT_MAX, img.max);
		cout << "New max " << img.max << endl;
		correct_vector(img.data, gamma, old_max, img.max, 0, img.data.size());
	} 
}

void split_send_receive(image& img, float gamma, int old_max, int size) {
	/*
	 * Divide the img data into several chunks. Then send the chunks too
	 * all the "slaves" for them to compute. The master takes care of the
	 * last chunk, which will be the biggest, since it takes care to
	 * compute to the end of the vector in order to cover for aproximations
	 * in the chunk_size computation
	 */
	size_t chunk_size = img.data.size() / size;
	// TODO investigate why this segfaults for a mad memory access. Try making i private
	#pragma omp parallel for default(shared)
	for(int i = 1; i < size; ++i) {
		MPI_Send(&chunk_size, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
		MPI_Send(&gamma, 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
		MPI_Send(&old_max, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
		MPI_Send(&img.max, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
		MPI_Send(&img.data[(i - 1) * chunk_size], chunk_size, MPI_INT, i, 0, MPI_COMM_WORLD);
	}
	size_t offset = (size - 1) * chunk_size;
	// compute the rest of the vector.
	correct_vector(img.data, gamma, old_max, img.max, offset, img.data.size());
	std::vector< int> temp(chunk_size);
	#pragma opm parallel for
	for(int i = 1, chunks = 0; i < size; ++i, ++chunks) {
		// receive the chunks
		MPI_Recv(&temp[0], chunk_size, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		// update the local image
		#pragma omp parallel for default(shared)
		for(size_t off = 0; off < chunk_size; ++off) {
			img.data[chunks * chunk_size + off] = temp[off];
		}
	}
}

void receive_compute_send(int rank) {
	int vector_size = 0;
	float sent_gamma = 1;
	int old_max = 0;
	int new_max = 0;
	int pid = getpid();
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	// Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

	// receive the gamma and the chunk size
	MPI_Recv(&vector_size, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&sent_gamma, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&old_max, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&new_max, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	// allocate a temporary vector of the right size
	std::vector<int> buffer(vector_size);
	MPI_Recv(&buffer[0], vector_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	// receive, compute and send back the computed vector
	correct_vector(buffer, sent_gamma, old_max, new_max, 0, vector_size);
	MPI_Send(&buffer[0], vector_size, MPI_INT, 0, 0, MPI_COMM_WORLD);

    // Print off a hello world message
    printf("PID %d@Processor %s, rank %d"
           " out of %d processors has finished the job\n",
           pid, processor_name, rank, size);
}

void correct_vector(std::vector<int>& data, float gamma, int old_max, int new_max, int start, int end) {
	#pragma omp parallel for shared(data)
	for(int i = start; i < end; ++i) {
		data[i] = (int) new_max * pow(data[i] / (double) old_max, gamma);
	}
}
