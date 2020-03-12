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

    template<typename Board>
    class acquisition_switch
    {
        Board const & b;
        
        bool start_adc() const noexcept
        {
            constexpr uint8_t start_adc_cmd = 1;
            return setup_command(b, []() noexcept {return (uint8_t)0;}, start_adc_cmd, "Start ADC command OK: ", (int) start_adc_cmd);
        }

        bool stop_adc() const noexcept
        {
            constexpr uint8_t stop_adc_cmd = 0;
            return setup_command(b, []() noexcept {return (uint8_t)0;}, stop_adc_cmd, "Stop ADC command OK: ", (int) stop_adc_cmd);
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
        void test()
        {
            typename Board::board_commander const & _ = b;
            _.test();
        }
        
        acquisition_switch(const class_type&) = delete;
        acquisition_switch& operator=(const class_type&) = delete;
        acquisition_switch(class_type&&) = delete;
        acquisition_switch& operator=(class_type&&) = delete;

    };
    
}

#endif /* BOARD_THREAD_H */

