/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   board.hpp
 * Author: wiluite
 *
 * Created on 27 февраля 2020 г., 13:16
 */

#if !defined (__linux__) || !defined (__arm__)
#error "unknown OS"
#endif

#ifndef BOARD_HPP
#define BOARD_HPP

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <unistd.h>
#include <string>

#include "board_functions.hpp"

namespace spi_adc_client
{
    class board
    {
    public:
        class board_error : public std::exception
        {
        protected:    
            mutable std::string msg {"board_error"};
        public:    
            board_error() = default;
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
                    close(handle_);                    
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

//--- board_commander        
        
        class board_commander
        {
            board const& b;
            
        public:    
            board_commander (board const & b) : b(b) {}
        
            void test() const {}
        };
        
        board_handle handle_;
        board_initializer initializer;
        board_commander commander;

    public:
        
        board() : initializer (handle_), commander(*this) {}
                
        decltype(initializer.bits) get_bits() const noexcept
        {
            return initializer.bits;
        }

        decltype(initializer.speed) get_speed() const noexcept
        {
            return initializer.speed;
        }
                
        operator int() const noexcept
        {
            return handle_;
        }
        
        operator board_commander const & ()
        {
            return commander;
        }
        
        template <class>
        friend class acquisition_switch;                       
    };
    
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

        acquisition_switch(Board const & b) : b(b) 
        {
            if (!start_adc())
            {
                Log_Wrapper("Can't start data acquisition!");
                throw acquisition_switch_start_exception();
            }
        }
        
        using class_type = acquisition_switch<Board>;
        
        acquisition_switch(const class_type&) = delete;
        acquisition_switch& operator=(const class_type&) = delete;
        acquisition_switch(class_type&&) = delete;
        acquisition_switch& operator=(class_type&&) = delete;
                
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
    };
    
}


#endif /* BOARD_HPP */

