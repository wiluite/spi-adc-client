/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   board_thread.h
 * Author: wiluite
 *
 * Created on 5 марта 2020 г., 9:35
 */

#ifndef BOARD_THREAD_H
#define BOARD_THREAD_H
#include <thread>
#include "board_functions.hpp"

namespace spi_adc_client
{
    static std::atomic<bool> io_flag{false};
    static uint8_t rcv_buf[std::numeric_limits<max_read_length_type>::max()+std::numeric_limits<max_read_length_type>::min()+1];
    static_assert(sizeof(rcv_buf)== 65536, "");

    template<typename Board>
    class acquisition_switch
    {
        Board const & b;
        
        bool start_adc() const noexcept
        {
            constexpr uint8_t start_adc_cmd = 1;
            return setup_command(b, []() noexcept {return (uint8_t)0;}, start_adc_cmd, "Start ADC command OK: ", (int) start_adc_cmd);
            return true;
        }

        bool stop_adc() const  noexcept
        {
            constexpr uint8_t stop_adc_cmd = 0;
            return setup_command(b, []() noexcept {return (uint8_t)0;}, stop_adc_cmd, "Stop ADC command OK: ", (int) stop_adc_cmd);
            return true;
        }
        
    public:
        struct acquisition_switch_exception : public std::exception
        {            
        };
        
        struct acquisition_switch_start_exception : public acquisition_switch_exception
        {            
        };

        using class_type = acquisition_switch<Board>;

        acquisition_switch(Board const & b) : b(b) 
        {
            if (!start_adc())
            {
                Log_Wrapper("Can't start data acquisition!");
                throw acquisition_switch_start_exception();
            }
        }
        
        ~acquisition_switch()
        {
            if (!stop_adc())
            {
                Log_Wrapper("Can't stop data acquisition!");     
            } else
            {
                Log_Wrapper("Data acquisition successfully stopped.");                
            }
        }
        
        acquisition_switch(const class_type&) = delete;
        acquisition_switch& operator=(const class_type&) = delete;
        acquisition_switch(class_type&&) = delete;
        acquisition_switch& operator=(class_type&&) = delete;

    };

    template<typename Board>
    void io_func(Board const & b)
    {
        try
        {
            acquisition_switch<Board> s(b);            
            io_flag = true;

            while (io_flag)
            {
                if (auto const len = read_buffer(b, rcv_buf, read_ready_flag_command(b)))
                {
                    static std::ofstream binary_file {"oscillogram.bin", std::ios::out | std::ios::binary};
                    binary_file.write ((char*)rcv_buf, len);                
                }
            }
            
        } catch (typename acquisition_switch<Board>::acquisition_switch_exception const & e)
        {
            Log_Wrapper("acquisition_switch_start_exception ", e.what());
            return;
        }
    }
    
}

#endif /* BOARD_THREAD_H */

