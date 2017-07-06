#ifndef NNET_LAYER_H_
#define NNET_LAYER_H_

#include "nnet_default.h"
#include "hls_stream.h"

namespace nnet {

template<class data_T, class res_T, class weight_T, class bias_T, class acc_T, int N_IN, int N_OUT>
void compute_small_layer(
    hls::stream<data_T>    &data,
    hls::stream<res_T>     &res,
    weight_T  weights[N_IN][N_OUT],
    bias_T    biases[N_OUT]);

template<class data_T, class res_T, class weight_T, class bias_T, class acc_T, int N_IN, int N_OUT>
void compute_medium_layer(
    hls::stream<data_T>    &data,
    hls::stream<res_T>     &res,
    weight_T  weights[N_IN][N_OUT],
    bias_T    biases[N_OUT]);

template<class data_T, class res_T, class weight_T, class bias_T, class acc_T, int N_IN, int N_OUT>
void compute_large_layer(
    hls::stream<data_T>    &data,
    hls::stream<res_T>     &res,
    weight_T  weights[N_IN][N_OUT],
    bias_T    biases[N_OUT]);

// *************************************************
//       Entry Function
// *************************************************

template<class data_T, class res_T, class weight_T, class bias_T, class acc_T, int N_IN, int N_OUT>
void compute_layer(
    hls::stream<data_T>    &data,
    hls::stream<res_T>     &res,
    weight_T  weights[N_IN][N_OUT],
    bias_T    biases[N_OUT])
{
    if (N_OUT >= 512) {
        compute_large_layer<data_T, res_T, weight_T, bias_T, acc_T, N_IN, N_OUT>(data, res, weights, biases);
    }
    else if (N_OUT >= 32) {
        compute_medium_layer<data_T, res_T, weight_T, bias_T, acc_T, N_IN, N_OUT>(data, res, weights, biases);
    }
    else {
        compute_small_layer<data_T, res_T, weight_T, bias_T, acc_T, N_IN, N_OUT>(data, res, weights, biases);
    }
}

// *************************************************
//       Possible implementation options
// *************************************************


template<class data_T, class res_T, class weight_T, class bias_T, class acc_T, int N_IN, int N_OUT>
void compute_small_layer(
    hls::stream<data_T>    &data,
    hls::stream<res_T>     &res,
    weight_T  weights[N_IN][N_OUT],
    bias_T    biases[N_OUT])
{
    data_T data_cache;
    acc_T acc[N_OUT];

	#pragma HLS ARRAY_RESHAPE variable=weights complete dim=2
    #pragma HLS ARRAY_PARTITION variable=acc complete dim=1

    Reset: for(int iacc = 0; iacc < N_OUT; iacc++)
    #pragma HLS UNROLL
        acc[iacc] = 0;

    NewInput: for(int ii = 0; ii < N_IN; ii++) {
    #pragma HLS PIPELINE
    	data_cache = data.read();
        Product: for(int jj = 0; jj < N_OUT; jj++) {
            acc[jj] += data_cache * weights[ii][jj];
        }
    }

    Result: for(int ires = 0; ires < N_OUT; ires++)
	#pragma HLS PIPELINE
        res << (res_T) (acc[ires] + (acc_T) biases[ires]);
}

template<class data_T, class res_T, class weight_T, class bias_T, class acc_T, int N_IN, int N_OUT>
void compute_medium_layer(
    hls::stream<data_T>    &data,
    hls::stream<res_T>     &res,
    weight_T  weights[N_IN][N_OUT],
    bias_T    biases[N_OUT])
{
    data_T data_cache;
    acc_T acc[N_OUT];

    #pragma HLS ARRAY_PARTITION variable=weights cyclic factor=8 dim=2
    #pragma HLS ARRAY_PARTITION variable=acc cyclic factor=8 dim=1

    // Optional... Cuts down on a few of the BRAMs
    #pragma HLS RESOURCE variable=acc core=RAM_2P_LUTRAM

    Reset: for(int iacc = 0; iacc < N_OUT; iacc++) {
    #pragma HLS UNROLL factor=8
        acc[iacc] = 0;
    }

    NewInput: for(int ii = 0; ii < N_IN; ii++) {
        data_cache = data.read();
        Product: for(int jj = 0; jj < N_OUT; jj++) {
        #pragma HLS UNROLL factor=8
        #pragma HLS PIPELINE
            acc[jj] += data_cache * weights[ii][jj];
        }
    }

    Result: for(int ires = 0; ires < N_OUT; ires++)
    #pragma HLS PIPELINE
        res << (res_T) (acc[ires] + (acc_T) biases[ires]);
}


template<class data_T, class res_T, class weight_T, class bias_T, class acc_T, int N_IN, int N_OUT>
void compute_large_layer(
    hls::stream<data_T>    &data,
    hls::stream<res_T>     &res,
    weight_T  weights[N_IN][N_OUT],
    bias_T    biases[N_OUT])
{
    data_T data_cache;
    acc_T acc[N_OUT];

    #pragma HLS ARRAY_PARTITION variable=weights cyclic factor=128 dim=2
    #pragma HLS ARRAY_PARTITION variable=acc cyclic factor=128 dim=1

    // Optional... Cuts down on a few of the BRAMs
    #pragma HLS RESOURCE variable=acc core=RAM_2P_LUTRAM

    Reset: for(int iacc = 0; iacc < N_OUT; iacc++) {
        #pragma HLS UNROLL factor=128
        acc[iacc] = 0;
    }

    NewInput: for(int ii = 0; ii < N_IN; ii++) {
        data_cache = data.read();
        Product: for(int jj = 0; jj < N_OUT; jj++) {
        #pragma HLS UNROLL factor=128
        #pragma HLS PIPELINE
            acc[jj] += data_cache * weights[ii][jj];
        }
    }

    Result: for(int ires = 0; ires < N_OUT; ires++)
    #pragma HLS PIPELINE
        res << (res_T) (acc[ires] + (acc_T) biases[ires]);
}

}

#endif
