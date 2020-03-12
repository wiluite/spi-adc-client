/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   board_functions.hpp
 * Author: wiluite
 *
 * Created on 27 февраля 2020 г., 14:48
 */

#ifndef BOARD_FUNCTIONS_HPP
#define BOARD_FUNCTIONS_HPP

#if !defined (__linux__) || !defined (__arm__)
#error "unknown OS"
#endif

#include <type_traits>
#include "log_wrapper.h"
#include <fstream>
#include <unordered_map>

namespace spi_adc_client
{
    template<typename B, typename F, typename ... T>
    static bool setup_command(B const & b, F const & param_fun, uint8_t cmd_number, T &&... args) noexcept
    {
        static_assert(std::is_same<uint8_t, decltype(param_fun())>::value, "");
        static uint8_t tx1[1] = {cmd_number};
        static decltype(param_fun()) tx2[2] = {param_fun(), 0};
        static uint8_t rx1[1], rx2[2];
        
        static struct spi_ioc_transfer tr[2] 
        {
            {(__u64)tx1, (__u64)rx1, sizeof(tx1), b.get_speed(), 500, b.get_bits(), 0,   0, 0, 0},
            {(__u64)tx2, (__u64)rx2, sizeof(tx2), b.get_speed(), 100, b.get_bits(), 0,   0, 0, 0}
        };

        if (ioctl(b, SPI_IOC_MESSAGE(2), tr) < 1)
        {
            Log_Wrapper ("Can't call SPI_IOC_MESSAGE(2) for command: ", static_cast<int>(cmd_number), __FILE__, " , ", __LINE__);
            return false;
        }
        
        if (((rx2[1] << 8) | rx2[0]) != cmd_number)
        {
            Log_Wrapper ("No reply or reply is invalid: ", static_cast<int>(cmd_number), __FILE__, " , ", __LINE__);
            return false;
        }
        
        Log_Wrapper (std::forward<T>(args) ...);

        return true;
    }
    
   
    template <typename Board>
    bool configure(Board const & b) noexcept
    {
        auto const ch_cnt_fun = []() noexcept 
        {
            constexpr uint8_t ch_count = 3;
            return ch_count;
        };

        auto const input_range_code_fun = []() noexcept
        {
            constexpr uint8_t SPI_RANGE_10 = 1;
            constexpr uint8_t SPI_RANGE_2 = 0;
            return SPI_RANGE_10; // no difference for imitator
        };
        
        auto const sample_size_fun = []() noexcept
        {
            using recv_sample_type = int32_t;
            return (uint8_t)sizeof(recv_sample_type);   
        };
        
        constexpr size_t channel_rate = 100000;
        auto const freq_code_fun = [=]() noexcept 
        {
            constexpr uint8_t ADC_SPI_FREQ_10K = 0;
            constexpr uint8_t ADC_SPI_FREQ_20K = 1;
            constexpr uint8_t ADC_SPI_FREQ_50K = 2;
            constexpr uint8_t ADC_SPI_FREQ_100K = 3;
                    
            static std::unordered_map<size_t, uint8_t> ch_rate_to_code = 
            {{10000, ADC_SPI_FREQ_10K}, {20000, ADC_SPI_FREQ_20K}, {50000, ADC_SPI_FREQ_50K}, {100000, ADC_SPI_FREQ_100K}};
                        
            return ch_rate_to_code[channel_rate];
        };
        
        
        constexpr uint8_t set_chan_cnt_cmd = 2;
        constexpr uint8_t set_input_range_cmd = 3;
        constexpr uint8_t set_sample_size_cmd = 4;
        constexpr uint8_t set_adc_cmd = 5;  
        
        return !(
                (!setup_command(b, ch_cnt_fun, set_chan_cnt_cmd, "Channel number setup command- OK: ", static_cast<int>(set_chan_cnt_cmd)
                , " number: ", static_cast<int>(ch_cnt_fun())))
                ||
                (!setup_command(b, input_range_code_fun, set_input_range_cmd, "Input range setup command- OK: ", static_cast<int>(set_input_range_cmd)
                , " range code: ", static_cast<int>(input_range_code_fun())))
                ||
                (!setup_command(b, sample_size_fun, set_sample_size_cmd, "Sample size setup command- OK: ", static_cast<int>(set_sample_size_cmd)
                , " size: ", static_cast<int>(sample_size_fun())))                
                ||
                (!setup_command(b, freq_code_fun, set_adc_cmd, "Channel sample rate setup command- OK: ", static_cast<int>(set_adc_cmd), " : "
                , channel_rate))
                );        
    }
    
    using max_read_length_type = uint16_t;
    
    template<typename Board>
    max_read_length_type read_ready_flag_command(Board const & b) noexcept
    {
        constexpr uint8_t cmd = 6;

        constexpr uint8_t tx1[1] = {cmd};
        constexpr uint8_t tx2[2] = {0xFF, 0xFF};
        static uint8_t rx1[1] {}, rx2[2] {};


        static struct spi_ioc_transfer tr[2] {
            {(__u64)tx1, (__u64)rx1, sizeof(tx1), b.get_speed(), 50, b.get_bits(), 0,   0, 0, 0},
            {(__u64)tx2, (__u64)rx2, sizeof(tx2), b.get_speed(), 0,  b.get_bits(), 0,   0, 0, 0}
        };

        if (ioctl(b, SPI_IOC_MESSAGE(2), tr) < 1)
        {
            Log_Wrapper ("Can't call SPI_IOC_MESSAGE(2) for command: ", static_cast<int>(cmd), __FILE__, " , ", __LINE__);
            return false;
        }

        return (rx2[1] << 8) | rx2[0];
    }


    template<typename Board>
    max_read_length_type read_buffer(Board const & b, uint8_t * const buf_ptr, max_read_length_type len) noexcept
    {
        static_assert(std::is_same<decltype(buf_ptr), uint8_t * const>::value, "");

        if (!len)
        {
            return 0;
        }

        static uint8_t dummy_tx_buf[std::numeric_limits<max_read_length_type>::max()+std::numeric_limits<max_read_length_type>::min()+1];
        
        static_assert(sizeof(dummy_tx_buf)== 65536, "");

        static struct spi_ioc_transfer tr[1] {
            {(__u64)dummy_tx_buf, (__u64)buf_ptr, len, b.get_speed(), 0, b.get_bits(), 0,   0, 0, 0}
        };

        tr[0].len = len;
                

        if (ioctl(b, SPI_IOC_MESSAGE(1), tr) < 1)
        {
            Log_Wrapper ("Can't send an SPI msg, len = ", len, __FILE__, " , ", __LINE__);
            return 0;
        } 

        return len;
    }

}


#endif /* BOARD_FUNCTIONS_HPP */

