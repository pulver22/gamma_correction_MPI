## Gamma correction using MPI
This code was written during the "Middleware Technologies for distributed System" master course. 
Its goal is to allowed gamma correction of an image using openMPI and openMP libraries. The first permits parallelism at singlecore level (multi-threading) while the second one at multicore (or cluster) level (multi-processing).

# How to compile
>mpiCC main.cc -fopenmp -o gamma

# How to run
> mpirun -n <numproc> ./gamma 'path_to_image' 'gamma_value'

It is possible to run the code on a cluster of different machine using the _--host_ parameter.



