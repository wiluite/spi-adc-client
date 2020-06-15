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
//#include <chrono>
#include <thread>
#include <fstream>

int main()
{
    using namespace spi_adc_client;
    try
    {
        board b;
        std::atomic io_flag {true};

        std::thread io([ & ]()
        {
            try
            {
                acquisition_switch s(b);
                while (io_flag)
                {
                    static uint8_t rcv_buf[std::numeric_limits<board::max_read_length_type>::max() + std::numeric_limits<board::max_read_length_type>::min() + 1];
                    static_assert(sizeof (rcv_buf) == 65536);

                    if (auto const len = b.read_buffer(rcv_buf))
                    {
                        static std::ofstream binary_file{"oscillogram.bin", std::ios::out | std::ios::binary};
                        binary_file.write((char const*) rcv_buf, len);
                    } else
                    {
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    }
                }
            } catch (acquisition_switch<board>::acquisition_switch_exception const & e) 
            {
                Log_Wrapper("acquisition_switch_start_exception ", e.what());
                return;
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        io_flag = false;
        io.join();
    } catch (board::board_error const & e)
    {
        Log_Wrapper(e.what());
        return -1;
    }
    return 0;
}

