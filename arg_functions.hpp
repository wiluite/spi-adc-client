/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   arg_functions.hpp
 * Author: wiluite
 *
 * Created on 27 февраля 2020 г., 13:45
 */

#ifndef ARG_FUNCTIONS_HPP
#define ARG_FUNCTIONS_HPP
#include <unordered_map>

namespace spi_adc_data
{    
    //constexpr uint8_t set_chan_cnt_cmd = 2;
    //constexpr uint8_t set_input_range_cmd = 3;
    //constexpr uint8_t set_sample_size_cmd = 4;
    //constexpr uint8_t set_adc_cmd = 5;  
 
    using recv_sample_type = int32_t;
    using max_read_length_type = uint16_t;
    
    constexpr size_t channel_rate = 100000;
    
    constexpr uint8_t ch_count = 3;
    
    constexpr size_t common_rate = channel_rate * ch_count;
    
    
}

namespace spi_adc_client
{
    #define SPI_RANGE_10 1
    #define SPI_RANGE_2 0
    
    uint8_t input_range_code() noexcept
    {
        return SPI_RANGE_10; // no difference for imitator
    }

    constexpr uint8_t sample_size() noexcept
    {
        return sizeof(spi_adc_data::recv_sample_type);
    };
        
}

#endif /* ARG_FUNCTIONS_HPP */

