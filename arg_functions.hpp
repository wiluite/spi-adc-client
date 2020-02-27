/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   arg_functions.hpp
 * Author: user
 *
 * Created on 27 февраля 2020 г., 13:45
 */

#ifndef ARG_FUNCTIONS_HPP
#define ARG_FUNCTIONS_HPP

namespace spi_adc_data
{    
    constexpr uint8_t set_chan_cnt_cmd = 2;
    constexpr uint8_t set_input_range_cmd = 3;
    constexpr uint8_t set_sample_size_cmd = 4;
    constexpr uint8_t set_adc_cmd = 5;  
 
    using recv_sample_type = int32_t;
    
    constexpr size_t channel_rate = 100000;
    
    constexpr uint8_t ch_count = 3;
    
    constexpr size_t common_rate = channel_rate * ch_count;
    
}

namespace spi_adc_client
{
    #define SPI_RANGE_10 1
    #define SPI_RANGE_2 0

    #define ADC_SPI_FREQ_10K  0
    #define ADC_SPI_FREQ_20K  1
    #define ADC_SPI_FREQ_50K  2
    #define ADC_SPI_FREQ_100K 3
    
    uint8_t freq_code() const noexcept
    {
        using spi_adc_data::channel_rate;
        
        static std::unordered_map<size_t, uint8_t> ch_rate_to_code = {{10000, ADC_SPI_FREQ_10K}, {20000, ADC_SPI_FREQ_20K}, {50000, ADC_SPI_FREQ_50K}, {100000, ADC_SPI_FREQ_100K}};
        return ch_rate_to_code[channel_rate];
    }
    
    uint8_t input_range_code() const noexcept
    {
        return SPI_RANGE_10; // no difference for imitator
    }

    constexpr uint8_t sample_size() const noexcept
    {
        return sizeof(spi_adc_data::recv_sample_type);
    };
    
    uint8_t ch_cnt() const noexcept
    {
        return spi_adc_data::ch_count;
    }
    
}

#endif /* ARG_FUNCTIONS_HPP */

