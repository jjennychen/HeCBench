#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <type_traits> // is_same
#include <cuda_fp16.h>
#include <cuda_runtime.h>
#include <cublas_v2.h>

using namespace std;

template <typename T>
void gemmBatched(
  int lower,
  int upper,
  int num,
  int reps,
  int verbose) 
{
  if(verbose) cout << "initializing inputs" << endl;
  size_t matrices_size = upper * upper * num * sizeof(T);
  size_t vectors_size = upper * num * sizeof(T);

  T *matrices = (T*)malloc(matrices_size);
  assert(matrices);

  T *vectors = (T*)malloc(vectors_size);
  assert(vectors);

  srand48(48);
  for(int i = 0; i < num * upper * upper; i++)
    matrices[i] = static_cast<T>(drand48());

  for(int i = 0; i < num * upper; i++)
    vectors[i] = static_cast<T>(drand48());

  cudaError_t cudaStat;
  cublasStatus_t stat;
  cublasHandle_t handle;

  stat = cublasCreate(&handle);
  if(stat != CUBLAS_STATUS_SUCCESS){
    cerr << "cublas init failed" << endl;
    exit(1);
  }

  if(verbose) cout << "allocating device variables" << endl;

  // allocate input space on device
  T *devMatrices;
  cudaStat = cudaMalloc((void**)&devMatrices, matrices_size);
  assert(!cudaStat);

  T *devVectors;
  cudaStat = cudaMalloc((void**)&devVectors, vectors_size);
  assert(!cudaStat);

  // allocate result space on device
  T *devResult;
  cudaStat = cudaMalloc((void**)&devResult, vectors_size);

  assert(!cudaStat);

  if(verbose) cout << "copying data to device" << endl;
  // copy data to device
  cudaStat = 
    cudaMemcpy(devMatrices, matrices, matrices_size, cudaMemcpyHostToDevice);

  assert(!cudaStat);
  
  cudaStat = 
    cudaMemcpy(devVectors, vectors, vectors_size, cudaMemcpyHostToDevice);

  assert(!cudaStat);

  // create lists of device pointers to inputs and outputs
  T **AList = 0, **BList = 0, **CList = 0;

  AList = (T**)malloc(num * sizeof(T*));
  BList = (T**)malloc(num * sizeof(T*));
  CList = (T**)malloc(num * sizeof(T*));

  int lda = upper, // lda >= max(1,m)
      ldb = upper, // ldb >= max(1,k)
      ldc = upper; // ldc >= max(1,m)

  const T alpha = 1.0f, beta = 0.0f;
  for(int i = 0; i < num; i++){
    // each array of dim. lda x k
    AList[i] = devMatrices + upper * upper * i;
    // each array of dim. ldb x n
    BList[i] = devVectors + upper * i;
    // each array of dim. ldc x n
    CList[i] = devResult + upper * i;
  }

  // copy pointer lists to device
  T **devAList, **devBList, **devCList;
  cudaStat = cudaMalloc((void**)&devAList, num * sizeof(T*));
  assert(!cudaStat);

  cudaStat = cudaMalloc((void**)&devBList, num * sizeof(T*));
  assert(!cudaStat);

  cudaStat = cudaMalloc((void**)&devCList, num * sizeof(T*));
  assert(!cudaStat);

  cudaStat = cudaMemcpy(devAList, AList, num * sizeof(T*), cudaMemcpyHostToDevice);
  assert(!cudaStat);
  
  cudaStat = cudaMemcpy(devBList, BList, num * sizeof(T*), cudaMemcpyHostToDevice);
  assert(!cudaStat);

  cudaStat = cudaMemcpy(devCList, CList, num * sizeof(T*), cudaMemcpyHostToDevice);
  assert(!cudaStat);


  /* perform <num> <size x size> x <size x 1> multiplications 
     with distinct matrices
   */
#define GEMM_BATCHED_PARAMETERS handle,              \
                                CUBLAS_OP_N,         \
                                CUBLAS_OP_N,         \
                                m, n, k,             \
                                &alpha,              \
                                (const T**)devAList, \
                                lda,                 \
                                (const T**)devBList, \
                                ldb,                 \
                                &beta,               \
                                devCList,            \
                                ldc,                 \
                                num

  for(int size = lower; size <= upper; size++){
    if(verbose) cout << "running with <size x size> x <size x 1> " << size << endl;
    double sum = 0.0;
    const int m = size, n = 1, k = size;
    for(int rep = 0; rep <= reps; rep++){
      auto start = std::chrono::steady_clock::now();

      if constexpr (std::is_same_v<T, double>)
        stat = cublasDgemmBatched(GEMM_BATCHED_PARAMETERS);
      else if constexpr (std::is_same_v<T, float>)
        stat = cublasSgemmBatched(GEMM_BATCHED_PARAMETERS);
      else if constexpr (std::is_same_v<T, __half>)
        stat = cublasHgemmBatched(GEMM_BATCHED_PARAMETERS);

      cudaDeviceSynchronize();
      auto end = std::chrono::steady_clock::now();
      auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
      auto elapsed = time * 1e-3;

      if(stat != CUBLAS_STATUS_SUCCESS){
        cerr << "cublasXgemmBatched failed" << endl;
        break;
      }

      if (rep != 0) sum += elapsed;
      
      if(verbose)
	cout << "size " << size << ": " << elapsed << " us; " 
	     << elapsed / num << " us per operation" << endl;
    }
    cout << "size " << size << " average execution time: " << sum/reps << " us; "
	 << sum / reps / num << " us per operation" << endl;
  }

  cudaFree(devMatrices);
  cudaFree(devVectors);
  cudaFree(devResult);
  cudaFree(devAList);
  cudaFree(devBList);
  cudaFree(devCList);

  free(matrices);
  free(vectors);
  free(AList);
  free(BList);
  free(CList);
}

int main(int argc, char ** argv){

  int status;
  int lower = 2;    // lower bound to the matrix dimension
  int upper = 100;  // upper bound to the matrix dimension
  int num = 25000;  // batch size
  int reps = 10;
  int verbose = 0;
  
  while((status = getopt(argc, argv, "l:u:n:r:v")) != -1){
    switch(status){
    case 'l':
      lower = strtoul(optarg, 0, 0);
      break;
    case 'u':
      upper = strtoul(optarg, 0, 0);
      break;
    case 'n':
      num = strtoul(optarg, 0, 0);  // batch size
      break;
    case 'r':
      reps = strtoul(optarg, 0, 0);
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      cerr << "invalid argument: " << status << endl;
      exit(1);
    }
  }

  cout << "running with" << " lower: " << lower << " upper: " << upper
       << " num: " << num << " reps: " << reps << endl;

  cout << ">>>>>>>>>>>>>>> Half precision gemmBatched >>>>>>>>>>>>>>> " << endl;
  gemmBatched<__half>(lower, upper, num, reps, verbose);
  cout << ">>>>>>>>>>>>>>> Single precision gemmBatched >>>>>>>>>>>>>>> " << endl;
  gemmBatched<float>(lower, upper, num, reps, verbose);
  cout << ">>>>>>>>>>>>>>> Double precision gemmBatched >>>>>>>>>>>>>>> " << endl;
  gemmBatched<double>(lower, upper, num, reps, verbose);
      
  return 0;
}