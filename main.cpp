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
           std::thread io (spi_adc_client::io_func<decltype(b)>, b);
           std::this_thread::sleep_for(std::chrono::milliseconds(10000));
           spi_adc_client::io_flag = false;
           io.join();
       }
    } catch(board::board_error const & e)
    {
       std::cout << e.what() << '\n';
    }
    return 0;
}

