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
#include "log_wrapper.h"
#include <unordered_map>
#include <type_traits>

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
        
        using max_read_length_type = uint16_t;
                
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
            explicit spi_mode_error(char const* m ) : board_error(m) {}
        };
        struct spi_bits_error : public board_error
        {
            explicit spi_bits_error(char const* m ) : board_error(m) {}
        };
        struct spi_speed_error : public board_error
        {
            explicit spi_speed_error(char const* m ) : board_error(m) {}
        };
        
        class board_initializer
        {
            uint32_t const mode = SPI_MODE_1; // CPOL=0 CPHA=1 (MSB by default!)           
            uint8_t const bits = 8;
            uint32_t const speed = 10000000;
        public:    
            board_handle const & bh_;
            explicit board_initializer (board_handle const & bh) : bh_(bh) 
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
        struct configure_command_error : public board_error
        {
            explicit configure_command_error(char const* m ) : board_error(m) {}
        };       
        
        class board_commander
        {
            board const& b;

            template<typename F, typename ... T>
            bool setup_command(F const & param_fun, uint8_t cmd_number, T &&... args) const noexcept 
            {
                static_assert(std::is_same<uint8_t, decltype(param_fun())>::value, "");
                static uint8_t tx1[1] = {cmd_number};
                static decltype(param_fun()) tx2[2] = {param_fun(), 0};
                static uint8_t rx1[1], rx2[2];

                static struct spi_ioc_transfer tr[2]
                {
                    {(__u64) tx1, (__u64) rx1, sizeof (tx1), b.get_speed(), 500, b.get_bits(), 0, 0, 0, 0},
                    {(__u64) tx2, (__u64) rx2, sizeof (tx2), b.get_speed(), 100, b.get_bits(), 0, 0, 0, 0}
                };

                if (ioctl(b, SPI_IOC_MESSAGE(2), tr) < 1) 
                {
                    Log_Wrapper("can't call SPI_IOC_MESSAGE(2) for command: ", static_cast<int> (cmd_number), ",", __FILE__, ",", __LINE__);
                    return false;
                }

                if (((rx2[1] << 8) | rx2[0]) != cmd_number) 
                {
                    Log_Wrapper("no reply to command or reply is invalid: ", static_cast<int> (cmd_number), ",", __FILE__, ",", __LINE__);
                    return false;
                }

                Log_Wrapper(std::forward<T>(args) ...);

                return true;
            }
                        
            bool configure() const noexcept 
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
                    return static_cast<uint8_t>(sizeof (recv_sample_type));
                };

                constexpr size_t channel_rate = 100000;
                auto const freq_code_fun = [ = ]() noexcept
                {
                    constexpr uint8_t ADC_SPI_FREQ_10K = 0;
                    constexpr uint8_t ADC_SPI_FREQ_20K = 1;
                    constexpr uint8_t ADC_SPI_FREQ_50K = 2;
                    constexpr uint8_t ADC_SPI_FREQ_100K = 3;

                    static std::unordered_map<size_t, uint8_t> ch_rate_to_code =
                    {
                        {10000, ADC_SPI_FREQ_10K},
                        {20000, ADC_SPI_FREQ_20K},
                        {50000, ADC_SPI_FREQ_50K},
                        {100000, ADC_SPI_FREQ_100K}
                    };

                    return ch_rate_to_code[channel_rate];
                };


                constexpr uint8_t set_chan_cnt_cmd = 2;
                constexpr uint8_t set_input_range_cmd = 3;
                constexpr uint8_t set_sample_size_cmd = 4;
                constexpr uint8_t set_adc_cmd = 5;

                return !(
                        (!setup_command(ch_cnt_fun, set_chan_cnt_cmd, "Channel number setup command - OK: "
                        , static_cast<int> (set_chan_cnt_cmd)
                        , " number: ", static_cast<int> (ch_cnt_fun())))
                        ||
                        (!setup_command(input_range_code_fun, set_input_range_cmd, "Input range setup command - OK: "
                        , static_cast<int> (set_input_range_cmd)
                        , " range code: ", static_cast<int> (input_range_code_fun())))
                        ||
                        (!setup_command(sample_size_fun, set_sample_size_cmd, "Sample size setup command - OK: "
                        , static_cast<int> (set_sample_size_cmd)
                        , " size: ", static_cast<int> (sample_size_fun())))
                        ||
                        (!setup_command(freq_code_fun, set_adc_cmd, "Channel sample rate setup command - OK: "
                        , static_cast<int> (set_adc_cmd), " : "
                        , channel_rate))
                        );
            }

            max_read_length_type read_ready_flag_command() const noexcept 
            {
                constexpr uint8_t cmd = 6;

                constexpr uint8_t tx1[1] = {cmd};
                constexpr uint8_t tx2[2] = {0xFF, 0xFF};
                static uint8_t rx1[1]{}, rx2[2]{};

                static struct spi_ioc_transfer tr[2]
                {
                    {(__u64) tx1, (__u64) rx1, sizeof (tx1), b.get_speed(), 50, b.get_bits(), 0, 0, 0, 0},
                    {(__u64) tx2, (__u64) rx2, sizeof (tx2), b.get_speed(), 0, b.get_bits(), 0, 0, 0, 0}
                };

                if (ioctl(b, SPI_IOC_MESSAGE(2), tr) < 1) 
                {
                    Log_Wrapper("can't call SPI_IOC_MESSAGE(2) for command: ", static_cast<int> (cmd), ",", __FILE__, ",", __LINE__);
                    return false;
                }

                return (rx2[1] << 8) | rx2[0];
            }
                        
        public:                
            explicit board_commander (board const & b) : b(b) 
            {
                if (!configure())
                    throw configure_command_error("can't configure board, check your imitator software.");
            }

            max_read_length_type read_buffer(uint8_t * const buf_ptr) const noexcept 
            {
                static_assert(std::is_same<decltype(buf_ptr), uint8_t * const>::value, "");

                if (auto const len = read_ready_flag_command()) 
                {                                   
                    static uint8_t dummy_tx_buf[std::numeric_limits<max_read_length_type>::max() + std::numeric_limits<max_read_length_type>::min() + 1];
                    static_assert(sizeof (dummy_tx_buf) == 65536, "");
                    
                    static struct spi_ioc_transfer tr[1]
                    {
                        {(__u64) dummy_tx_buf, (__u64) buf_ptr, len, b.get_speed(), 0, b.get_bits(), 0, 0, 0, 0}
                    };

                    tr[0].len = len;

                    if (ioctl(b, SPI_IOC_MESSAGE(1), tr) < 1) 
                    {
                        Log_Wrapper("can't send an SPI msg, len = ", len, ",", __FILE__, ",", __LINE__);
                        return 0;
                    }                    
                    return len;                    
                } else
                {
                    return 0;
                }
            }           
            
            template <class>
            friend class acquisition_switch;            
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
        
        operator board_commander const & () const noexcept
        {
            return commander;
        }
                
        auto read_buffer(uint8_t * const buf_ptr)
        {
            return commander.read_buffer(buf_ptr);
        }        
        
        template <class>
        friend class acquisition_switch;        
    };
    
    template<typename Board>
    class acquisition_switch
    {
        Board const & b;
        typename Board::board_commander const & bc;
        
        bool start_adc() const noexcept
        {
            constexpr uint8_t start_adc_cmd = 1;
            return bc.setup_command([]() noexcept {return (uint8_t)0;}, start_adc_cmd, "Start ADC command - OK: ", static_cast<int>(start_adc_cmd));
        }

        bool stop_adc() const noexcept
        {
            constexpr uint8_t stop_adc_cmd = 0;
            return bc.setup_command([]() noexcept {return (uint8_t)0;}, stop_adc_cmd, "Stop ADC command - OK: ", static_cast<int>(stop_adc_cmd));
        }
        
    public:
        struct acquisition_switch_exception : public Board::board_error
        {            
            explicit acquisition_switch_exception(char const* m ) : Board::board_error(m) {}
        };
        
        struct acquisition_switch_start_exception : public acquisition_switch_exception
        {           
            explicit acquisition_switch_start_exception(char const* m ) : acquisition_switch_exception(m) {}
        };

        explicit acquisition_switch(Board const & b) : b(b), bc(b)
        {
            if (!start_adc())
            {
                throw acquisition_switch_start_exception("can't start data acquisition!");
            }
        }
        
        using class_type = acquisition_switch<Board>;
        
        acquisition_switch(const class_type&) = delete;
        acquisition_switch& operator=(const class_type&) = delete;
        acquisition_switch(class_type&&) = delete;
        acquisition_switch& operator=(class_type&&) = delete;
                
        ~acquisition_switch() noexcept
        {
            if (!stop_adc())
            {
                Log_Wrapper("can't stop data acquisition!");     
            } else
            {
                Log_Wrapper("Data acquisition successfully stopped.");                
            }
        }        
    };
    
}


#endif /* BOARD_HPP */

