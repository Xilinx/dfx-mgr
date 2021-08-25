/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 * 
 * SPDX-License-Identifier: MIT
 */

/*
 * @file    device.h
 * @brief   Accelerator devices
 */
 
#ifndef DEVICE_HPP_
#define DEVICE_HPP_

typedef struct DeviceConfig {
    char name[128];
    //acapd_device_t *dev; /**< pointer to the DMA device */
    //int chnl_id; /**< hardware channel id of a data mover controller */
    //acapd_dir_t dir; /**< DMA channel direction */
    //uint32_t conn_type; /**< type of data connection with this channel */
    //void *sys_info; /**< System private data for the channel */
    //int is_open; /**< Indicate if the channel is open */
    //acapd_dma_ops_t *ops; /**< DMA operations */
    //acapd_list_t node; /**< list node */
    //char *bd_va;
    //uint32_t max_buf_size;
} DeviceConfig_t;


/* Accelerator devices interface */
typedef struct Device {
    int (*open )(DeviceConfig_t *config);
    int (*close)(DeviceConfig_t *config);
    //int (*transfer)(acapd_chnl_t *chnl, acapd_dma_config_t *config);
    //int (*stop)(acapd_chnl_t *chnl);
    //acapd_chnl_status_t (*poll)(acapd_chnl_t *chnl);
    //int (*reset)(acapd_chnl_t *chnl);
    } Device_t;


typedef int (*REGISTER)(Device_t**, DeviceConfig_t**);
typedef int (*UNREGISTER)(Device_t**, DeviceConfig_t**);

#endif // DEVICE_HPP_
