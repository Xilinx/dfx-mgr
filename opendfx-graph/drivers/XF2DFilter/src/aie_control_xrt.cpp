#include <iostream>
#include "adf/adf_api/AIEControlConfig.h"


/************************** Graph Configurations  *****************************/

  adf::GraphConfig GraphConfigurations[] = {
  //{id, name, graphLoadElfFunc, graphInitFunc, graphDebugHalt, coreColumns, coreRows, iterMemColumns, iterMemRows, iterMemAddrs, triggered, plKernelInstanceNames, plAxiLiteModes, plDriverStartFuncs, plDriverCheckIPDoneFuncs}
    {0, "filter_graph", nullptr, nullptr, nullptr, {27}, {0}, {27}, {1}, {0x2004}, {0}, {}, {}, {}, {},  }, 
  };
  const int NUM_GRAPH = 1;


/************************** RTP Configurations  *****************************/

  adf::RTPConfig RTPConfigurations[] = {
  //{portId, aliasId, portName, aliasName, graphId, isInput, isAsync, isConnect, elemType, numBytes, isPL, hasLock, selectorColumn, selectorRow, selectorAddr, selectorLockId, pingColumn, pingRow, pingAddr, pongColumn, pongRow, pongAddr, pongLockId, plKernelInstanceName, plParameterIndex, plDriverWriteRTP, plDriverReadRTP}
    {6, 4, "filter_graph.k1.in[1]", "filter_graph.kernelCoefficients", 0, true, true, false, (adf::RTPConfig::elementType)2, 32, false, true, 27, 1, 0, 0, 27, 1, 0x6000, 1, 27, 1, 0x4000, 2, "", -1, nullptr, nullptr},
  };
  const int NUM_RTP = 1;


/************************** GMIO Configurations  *****************************/

  adf::GMIOConfig GMIOConfigurations[] = {
  //{id, name, loginal_name, type, shim_column, c_rts_channel_num (0-S2MM0,1-S2MM1,2-MM2S0,3-MM2S1), streamId, burst_length, plKernelInstanceName, plParameterIndex, plId (for aiesim), pl_driver_to_set_maxi_addr}
    {0, "gmioIn[0]", "gmioIn1", (adf::gmio_config::gmio_type)0, 27, 2, 3, 16, "", -1, -1, nullptr},
    {1, "gmioOut[0]", "gmioOut1", (adf::gmio_config::gmio_type)1, 27, 0, 2, 16, "", -1, -1, nullptr},
  };
  const int NUM_GMIO = 2;


/************************** ADF API initializer *****************************/

  class InitializeAIEControlXRT
  {
  public:
    InitializeAIEControlXRT()
    {
      std::cout<<"Initializing ADF API..."<<std::endl;
#ifdef __EXCLUDE_PL_CONTROL__
      bool exclude_pl_control = true;
#else
      bool exclude_pl_control = false;
#endif
      adf::initializeConfigurations(nullptr, 0, 0, 0,
                                    GraphConfigurations, NUM_GRAPH,
                                    RTPConfigurations, NUM_RTP,
                                    nullptr, 0,
                                    nullptr, 0,
                                    GMIOConfigurations, NUM_GMIO,
                                    nullptr, 0,
                                    nullptr, 0, 0, nullptr,
                                    false, exclude_pl_control, false, nullptr
                                    , false);

    }
  } initAIEControlXRT;



#if !defined(__CDO__)

// Kernel Stub Definition
  void filter2D(input_window<short> *,short const (&)[16],output_window<short> *) { /* Stub */ } 
#endif
