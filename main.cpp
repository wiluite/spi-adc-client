/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: wiluite
 *
 * Created on 27 февраля 2020 г., 8:05
 */

#include "board.hpp"
#include "board_thread.h"
#include <chrono>

int main(int argc, char** argv) {

    using spi_adc_client::board;
    using spi_adc_client::configure;
    try
    {
       board b;
       if (configure( b ))
       {
            static std::atomic<bool> io_flag{false};
            static uint8_t rcv_buf[std::numeric_limits<spi_adc_client::max_read_length_type>::max()+std::numeric_limits<spi_adc_client::max_read_length_type>::min()+1];
            static_assert(sizeof(rcv_buf)== 65536, "");

            std::thread io ([&b]() 
            {
                try
                {
                    spi_adc_client::acquisition_switch<board> s(b);        
                    s.test();
                    io_flag = true;
                    while (io_flag)
                    {
                        if (auto const len = read_buffer(b, rcv_buf, read_ready_flag_command(b)))
                        {
                            static std::ofstream binary_file {"oscillogram.bin", std::ios::out | std::ios::binary};
                            binary_file.write ((char*)rcv_buf, len);                
                        } else
                        {
                            std::this_thread::sleep_for(std::chrono::microseconds(10));
                        }
                    }
                } catch(typename spi_adc_client::acquisition_switch<board>::acquisition_switch_exception const & e)
                {
                    Log_Wrapper("acquisition_switch_start_exception ", e.what());
                    return;
                }
           });
           
           std::this_thread::sleep_for(std::chrono::milliseconds(5000));
           io_flag = false;
           io.join();
       }
    } catch(board::board_error const & e)
    {
       std::cout << e.what() << '\n';
    }
    return 0;
}

