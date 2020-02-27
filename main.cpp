/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: user
 *
 * Created on 27 февраля 2020 г., 8:05
 */
#if !defined (__linux__)
#error "unknown OS"
#endif

#include <cstdlib>

#include <iostream>
//#include <string>
#include "board.hpp"



int main(int argc, char** argv) {

    using spi_adc_client::board;
    try
    {
       board b;
    } catch(board::board_error const & e)
    {
       std::cout<< e.what() << std::endl; 
    }
    return 0;
}

