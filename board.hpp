/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   board.hpp
 * Author: user
 *
 * Created on 27 февраля 2020 г., 13:16
 */

#if !defined (__linux__)
#error "unknown OS"
#endif

#ifndef BOARD_HPP
#define BOARD_HPP

#include <fcntl.h>
#include <sys/ioctl.h>
//#include <linux/ioctl.h>
//#include <sys/stat.h>
//#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <unistd.h>


namespace spi_adc_client
{
    class board
    {
    public:
        struct board_error : public std::exception
        {
            mutable std::string msg {"board_error"};
            board_error() {}
            explicit board_error(char const * m) : msg(m) {}
            const char* what() const noexcept override
            {
                return msg.c_str();
            }
        };
        
        
    private:    
        
//--- board_handle
        class spi_device_opening_error : public board_error
        {
            int h;
        public:    
            explicit spi_device_opening_error(int h) : h(h) {}
            const char* what() const noexcept override
            {
                msg = std::string("spi_device_opening_error: ") + std::to_string(h); 
                return msg.c_str();
            }            
        };
        
        class board_handle
        {
            int handle_;
        public:    
            board_handle() : handle_(open("/dev/spidev1.0", O_RDWR)) 
            {
                if (handle_ < 0)
                {
                    throw spi_device_opening_error(handle_);
                }
            }
            
            ~board_handle() noexcept
            {
                if (handle_ >= 0)
                {
                    ::close(handle_);                    
                }
            }
            
            operator int() const noexcept
            {
                return handle_;
            }
        };
        
//--- board_initializer        
        struct spi_mode_error : public board_error
        {
            spi_mode_error(char const* m ) : board_error(m) {}
        };
        struct spi_bits_error : public board_error
        {
            spi_bits_error(char const* m ) : board_error(m) {}
        };
        struct spi_speed_error : public board_error
        {
            spi_speed_error(char const* m ) : board_error(m) {}
        };
        
        class board_initializer
        {
            uint32_t const mode = SPI_MODE_1; // CPOL=0 CPHA=1 (MSB by default!)           
            uint8_t const bits = 8;
            uint32_t const speed = 10000000;
        public:    
            board_handle const & bh_;
            board_initializer (board_handle const & bh) : bh_(bh) 
            {
                if (-1 == ioctl(bh_, SPI_IOC_WR_MODE, &mode))
                {
                    throw spi_mode_error("can't set spi mode");
                }
                if (-1 == ioctl(bh_, SPI_IOC_RD_MODE, &mode))
                {
                    throw spi_mode_error("can't get spi mode");
                }            
                if (-1 == ioctl(bh_, SPI_IOC_WR_BITS_PER_WORD, &bits))
                {
                    throw spi_bits_error("can't set spi bits");
                }
                if (-1 == ioctl(bh_, SPI_IOC_RD_BITS_PER_WORD, &bits))
                {
                    throw spi_bits_error("can't get spi bits");
                }
                if (-1 == ioctl(bh_, SPI_IOC_WR_MAX_SPEED_HZ, &speed))
                {
                    throw spi_speed_error("can't set spi speed");
                }
                if (-1 == ioctl(bh_, SPI_IOC_RD_MAX_SPEED_HZ, &speed))
                {
                    throw spi_speed_error("can't get spi speed");
                }                            
            }
            
            friend class board;
        };
    public:
        board_handle handle;
        board_initializer initializer;
        
        board() : initializer (handle) {}
                
        decltype(initializer.bits) get_bits() const
        {
            return initializer.bits;
        }

        decltype(initializer.speed) get_speed() const
        {
            return initializer.speed;
        }
                
    };
}


#endif /* BOARD_HPP */

