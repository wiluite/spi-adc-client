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
#include <utility> // for std::move()
#include <sys/file.h> //flock

namespace spi_adc_client
{
    class board
    {
    public:
        class board_error : public std::exception
        {
        protected:
            // TODO: turn to char const*
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
//--- board_protector
        class board_protector
        {
            int fd;
            constexpr static char const * const path = "/var/run/board.lock";
            class spi_device_protecting_error : public board_error
            {
            public:
                const char* what() const noexcept override {
                    msg = "spi_device_protecting_error";
                    return msg.c_str();
                }
            };

        public:
            board_protector()
            {
                fd = open(path, O_RDONLY | O_NOCTTY | O_CREAT);
                if ((fd < 0) || (flock(fd, LOCK_EX | LOCK_NB) < 0))
                {
                    // unnecessary check and action
                    if (fd >= 0)
                    {
                        close(fd);
                    }
                    throw spi_device_protecting_error();
                }
            }

            board_protector (board_protector&& bp) noexcept : fd(bp.fd)
            {
                bp.fd = -1;
            }

            // TODO: probably "delete"
            board_protector& operator= (board_protector&& bp) noexcept
            {
                if (this != &bp)
                {
                    fd = bp.fd;
                    bp.fd = -1;
                }
                return *this;
            }

            ~board_protector() noexcept
            {
                // unnecessary actions at neither actual nor move-from object
                if (fd >= 0)
                {
                    flock(fd, LOCK_UN);
                    close(fd);
                }
            }
        };

//--- board_handle
        class board_handle
        {
        public:
            using device_handle_type = int;
        private:
            class spi_device_opening_error : public board_error 
            {
                device_handle_type h;
            public:

                explicit spi_device_opening_error(device_handle_type h) : h(h) {}

                const char* what() const noexcept override {
                    msg = std::string("spi_device_opening_error: ") + std::to_string(h);
                    return msg.c_str();
                }
            };

            device_handle_type handle_;
            bool moved_from {false}; // special case!
        public:
            board_handle() : handle_(open("/dev/spidev1.0", O_RDWR))
            {
                if (handle_ < 0)
                {
                    throw spi_device_opening_error(handle_);
                }
            }

            board_handle (board_handle&& bh) noexcept : handle_(bh.handle_)
            {
                // NOTE: we can't call bh.handle_ = -1, because bh.handle_ is still needed by board_initializer's move constructor
                bh.moved_from = true;
                moved_from = false;
            }

            // TODO: probably "delete"
            board_handle& operator= (board_handle&& bh) noexcept
            {
                if (this != &bh)
                {
                    handle_ = bh.handle_;
                    // NOTE: we can't call bh.handle_ = -1, because bh.handle_ is still needed by board_initializer's move assignment operator
                    bh.moved_from = true;                    
                    moved_from = false;
                }
                return *this;
            }

            ~board_handle() noexcept
            {
                if (moved_from)
                    return;
                // we always have handle_ >= 0 here! No need checking
                close(handle_);
            }

            operator device_handle_type() const noexcept
            {
                return handle_;
            }
            
        };

//--- board_initializer
        class board_initializer
        {
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

            uint32_t mode = SPI_MODE_1; // CPOL=0 CPHA=1 (MSB by default!)
            uint8_t bits = 8;
            uint32_t speed = 10000000;
        public:
            board_handle & bh_;
            explicit board_initializer (board_handle & bh) : bh_(bh)
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
            board_initializer (board_initializer&& ) /*noexcept*/ = default;
            board_initializer& operator= (board_initializer&& ) /*noexcept*/ = default;

            friend class board;
        };
        
        auto get_bits() const noexcept
        {
            return initializer.bits;
        }
        
        auto get_speed() const noexcept
        {
            return initializer.speed;
        }
        

//--- board_commander
        class board_commander
        {
            struct configure_command_error : public board_error
            {
                explicit configure_command_error(char const* m ) : board_error(m) {}
            };

            board & b;

            template<typename F, typename ... T>
            auto setup_command(F const & param_fun, uint8_t cmd_number, T &&... args) const noexcept 
            {
                static_assert(std::is_same<uint8_t, decltype(param_fun())>::value);
                static uint8_t tx1[1] = {cmd_number};
                static decltype(param_fun()) tx2[2] = {param_fun(), 0};
                static uint8_t rx1[1], rx2[2];

                static struct spi_ioc_transfer tr[2]
                {
                    {(__u64) tx1, (__u64) rx1, sizeof (tx1), b.get_speed(), 50, b.get_bits(), 0, 0, 0, 0},
                    {(__u64) tx2, (__u64) rx2, sizeof (tx2), b.get_speed(), 50, b.get_bits(), 0, 0, 0, 0}
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
                auto constexpr ch_cnt_fun = []() noexcept
                {
                    constexpr uint8_t ch_count = 3;
                    return ch_count;
                };

                auto constexpr input_range_code_fun = []() noexcept
                {
                    constexpr uint8_t SPI_RANGE_10 = 1;
//                    constexpr uint8_t SPI_RANGE_2 = 0;
                    return SPI_RANGE_10; // no difference for imitator
                };

                auto constexpr sample_size_fun = []() noexcept
                {
                    using recv_sample_type = int32_t;
                    return static_cast<uint8_t>(sizeof (recv_sample_type));
                };

                constexpr size_t channel_rate = 100000;
                auto constexpr freq_code_fun = [ = ]() noexcept
                {
                    constexpr uint8_t ADC_SPI_FREQ_10K = 0;
                    constexpr uint8_t ADC_SPI_FREQ_20K = 1;
                    constexpr uint8_t ADC_SPI_FREQ_50K = 2;
                    constexpr uint8_t ADC_SPI_FREQ_100K = 3;

                    std::unordered_map<size_t, uint8_t> ch_rate_to_code =
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

                return 
                        (setup_command(ch_cnt_fun, set_chan_cnt_cmd, "Channel number setup command - OK: "
                        , static_cast<int> (set_chan_cnt_cmd)
                        , " number: ", static_cast<int> (ch_cnt_fun())))
                        &&
                        (setup_command(input_range_code_fun, set_input_range_cmd, "Input range setup command - OK: "
                        , static_cast<int> (set_input_range_cmd)
                        , " range code: ", static_cast<int> (input_range_code_fun())))
                        &&
                        (setup_command(sample_size_fun, set_sample_size_cmd, "Sample size setup command - OK: "
                        , static_cast<int> (set_sample_size_cmd)
                        , " size: ", static_cast<int> (sample_size_fun())))
                        &&
                        (setup_command(freq_code_fun, set_adc_cmd, "Channel sample rate setup command - OK: "
                        , static_cast<int> (set_adc_cmd), " : "
                        , channel_rate))
                        ;
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
                    return 0;
                }

                return (rx2[1] << 8) | rx2[0];
            }

        public:
            explicit board_commander (board & b) : b(b)
            {
                if (!configure())
                    throw configure_command_error("can't configure board, check your imitator software.");
            }

            board_commander (board_commander&& ) /*noexcept*/ = default;
            board_commander& operator= (board_commander&& ) /*noexcept*/ = default;
            
            max_read_length_type read_buffer(uint8_t * const buf_ptr) const noexcept 
            {
                static_assert(std::is_same<decltype(buf_ptr), uint8_t * const>::value, "");

                if (auto const len = read_ready_flag_command()) 
                {
                    static uint8_t dummy_tx_buf[std::numeric_limits<max_read_length_type>::max() + std::numeric_limits<max_read_length_type>::min() + 1];
                    static_assert(sizeof (dummy_tx_buf) == 65536);

                    static struct spi_ioc_transfer tr[1]
                    {
                        {(__u64) dummy_tx_buf, (__u64) buf_ptr, len, b.get_speed(), 0, b.get_bits(), 0, 0, 0, 0}
                    };

                    tr[0].len = len;

                    if (ioctl(b, SPI_IOC_MESSAGE(1), tr) < 1) 
                    {
                        Log_Wrapper("can't send an SPI msg, len = ", len, ",", __FILE__, ",", __LINE__);
                        return 0u;
                    }
                    return len;
                } else
                {
                    // maybe throw an exception?
                    return 0u;
                }
            }

            template <class>
            friend class acquisition_switch;
        };

        board_protector protector;
        board_handle handle_;
        board_initializer initializer;
        board_commander commander;

    public:

        // TODO: configure with parameters to run
        board() : /*protector(), handle_(),*/ initializer (handle_), commander(*this) {}
        
        board (board&& b) noexcept = default;
        board& operator= (board&& b) noexcept = default;
        
        operator board_handle::device_handle_type() const noexcept
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

        void start_adc() const
        {
            constexpr uint8_t start_adc_cmd = 1;
            if (!bc.setup_command([]() noexcept {return (uint8_t)0;}, start_adc_cmd, "Start ADC command - OK: ", static_cast<int>(start_adc_cmd)))
                throw acquisition_switch_start_exception("can't start data acquisition!");
        }

        void stop_adc() const noexcept
        {
            constexpr uint8_t stop_adc_cmd = 0;
            if (!bc.setup_command([]() noexcept {return (uint8_t)0;}, stop_adc_cmd, "Stop ADC command - OK: ", static_cast<int>(stop_adc_cmd)))
                Log_Wrapper("can't stop data acquisition!");
            else
                Log_Wrapper("Data acquisition successfully stopped.");
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
            start_adc();
        }

        using class_type = acquisition_switch<Board>;

        acquisition_switch(const class_type&) = delete; // disable copy (and move) semantics
        acquisition_switch& operator=(const class_type&) = delete; // disable copy (and move) semantics

        ~acquisition_switch() noexcept
        {
            stop_adc();
        }
    };
}


#endif /* BOARD_HPP */

