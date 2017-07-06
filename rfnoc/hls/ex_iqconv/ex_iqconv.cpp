
#include "nnet_conv.h"
#include "nnet_activation.h"
#include "nnet_layer.h"
#include "nnet_helpers.h"
#include "nnet_pooling.h"

#include "ex_iqconv.h"

void init_w1(weight_t w1[N_IQCONV_FILT][2*N_IQCONV_CHAN]){
  for (int ii=0; ii<N_IQCONV_FILT; ii++){
    for (int jj=0; jj<N_IQCONV_CHAN; jj++){
      w1[ii][2*jj]=0.101;
      w1[ii][2*jj+1]=-0.1;
    }
  }
}
void init_b1(bias_t b1[N_IQCONV_CHAN]){
  for (int ii=0; ii<N_IQCONV_CHAN; ii++) b1[ii]=ii*0.01;
}
void init_w2(weight_t w2[N_1DCONV_FILT][N_1DCONV_CHAN]){
  for (int ii=0; ii<N_1DCONV_FILT; ii++){
    for (int jj=0; jj<N_1DCONV_CHAN; jj++){
      w2[ii][jj]=0.1;
    }
  }
}
void init_b2(bias_t b2[N_1DCONV_CHAN]){
  for (int ii=0; ii<N_1DCONV_CHAN; ii++) b2[ii]=ii*0.01;
}


// AXI-Stream port type is compatible with pointer, reference, & array input / ouputs only
// See UG902 Vivado High Level Synthesis guide (2014.4) pg 157 Figure 1-49
void ex_iqconv(
      hls::stream<input_t> &data_i,
      hls::stream<input_t> &data_q,
      hls::stream<result_t> &res,
      unsigned short &const_size_in,
      unsigned short &const_size_out)
{
    // Initialize CONV weights
    weight_t w1 [N_IQCONV_FILT][2*N_IQCONV_CHAN];
    bias_t   b1 [N_IQCONV_CHAN];
    init_w1(w1);
    init_b1(b1);

    weight_t w2 [N_1DCONV_FILT][N_1DCONV_CHAN];
    bias_t   b2 [N_1DCONV_CHAN];
    init_w2(w2);
    init_b2(b2);

    // // Initialize DENSE weights
    // weight_t dw1 [N_LAYER_IN][N_LAYER_1];
    // bias_t   db1 [N_LAYER_1];
    // init_dw1(Dw1);
    // init_db1(Db1);

    // weight_t dw2 [N_LAYER_1][N_LAYER_2];
    // bias_t   db2 [N_LAYER_2];
    // init_dw2(dw2);
    // init_db2(db2);

    // weight_t dw3 [N_LAYER_2][N_LAYER_3];
    // bias_t   db3 [N_LAYER_3];
    // init_dw2(dw3);
    // init_db2(db3);

    // weight_t dw4 [N_LAYER_3][N_LAYER_4];
    // bias_t   db4 [N_LAYER_4];
    // init_dw2(dw4);
    // init_db2(db4);

    // Remove ap ctrl ports (ap_start, ap_ready, ap_idle, etc) since we only use the AXI-Stream ports
    #pragma HLS INTERFACE ap_ctrl_none port=return

    // Connect size indicators
    #pragma HLS INTERFACE axis port=data_i
    #pragma HLS INTERFACE axis port=data_q
    #pragma HLS INTERFACE axis port=res
    #pragma HLS INTERFACE ap_none port=const_size_in
    #pragma HLS INTERFACE ap_none port=const_size_out
    const_size_in   = N_LAYER_IN;
    const_size_out  = N_LAYER_OUT;

    #pragma HLS DATAFLOW

    // Optional debug printing-- insert inline to print data
    // hls::stream<layerIQ_t> dbg1;
    // nnet::hls_stream_debug<layerIQ_t, N_MAXPOOL_FLAT>(maxpool1, dbg1); 

    // ****************************************
    // NETWORK INSTATIATION
    // ****************************************

    // IQ Convolution
    hls::stream<layerIQ_t> conv1, hidden1;
    nnet::conv_iq<input_t, layerIQ_t, weight_t, bias_t, accum1_t, N_IN, N_IQCONV_FILT, N_IQCONV_CHAN>
                 (data_i, data_q, conv1, w1, b1);
    nnet::relu<layerIQ_t, layerIQ_t, N_IQCONV_FLAT>(conv1, hidden1);

    // Maxpool operation
    hls::stream<layerIQ_t> maxpool1;
    nnet::maxpool_2x<layerIQ_t, N_IQCONV_OUT, N_IQCONV_CHAN>(hidden1, maxpool1);

    // 1-Dimensional, Multichannel Convolution
    hls::stream<layer1D_t> conv2, hidden2;
    nnet::conv_1d<layerIQ_t, layer1D_t, weight_t, bias_t, accum2_t, N_1DCONV_IN, N_1DCONV_FILT, N_1DCONV_CHAN>
                 (maxpool1, conv2, w2, b2);
    nnet::relu<layer1D_t, layer1D_t, N_1DCONV_FLAT>(conv2, hidden2);

    // Maxpool operation
    hls::stream<layer1D_t> maxpool2;
    nnet::maxpool_2x<layer1D_t, N_1DCONV_OUT, N_1DCONV_CHAN>(hidden2, res);


    // // DENSE LAYER 1
    // hls::stream<layer1_t> dense1, relu1;
    // nnet::compute_layer<layer1D_t, layer1_t, weight_t, bias_t, accumX_t, N_LAYER_IN, N_LAYER_1>(maxpool2, dense1, w1, b1);
    // nnet::relu6<layer1_t, layer1_t, N_LAYER_1>(dense1, relu1);

    // // DENSE LAYER 2
    // hls::stream<layer2_t> dense2, relu2;
    // nnet::compute_layer<layer1_t, layer2_t, weight_t, bias_t, accumX_t, N_LAYER_1, N_LAYER_2>(relu1, dense2, w2, b2);
    // nnet::relu6<layer2_t, layer2_t, N_LAYER_2>(dense2, relu2);

    // // DENSE LAYER 3
    // hls::stream<layer3_t> dense3, relu3;
    // nnet::compute_layer<layer2_t, layer3_t, weight_t, bias_t, accumX_t, N_LAYER_2, N_LAYER_3>(relu2, dense3, w3, b3);
    // nnet::relu6<layer3_t, layer3_t, N_LAYER_3>(dense3, relu3);

    // // DENSE LAYER 4
    // hls::stream<layer4_t> dense4, relu4;
    // nnet::compute_layer<layer3_t, layer4_t, weight_t, bias_t, accumX_t, N_LAYER_3, N_LAYER_4>(relu3, dense4, w4, b4);
}
