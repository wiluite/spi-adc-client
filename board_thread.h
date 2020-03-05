/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   board_thread.h
 * Author: user
 *
 * Created on 5 марта 2020 г., 9:35
 */

#ifndef BOARD_THREAD_H
#define BOARD_THREAD_H
#include <thread>

namespace spi_adc_client
{
    static std::thread spi_thread;
    static std::atomic<bool> io_flag{false};
//    static uint8_t rcv_buf[100000];
    static uint8_t rcv_buf[std::numeric_limits<max_read_length_type>::max()+std::numeric_limits<max_read_length_type>::min()+1];
    static_assert(sizeof(rcv_buf)== 65536, "");

    template<typename PO>
    class SPIADCStarter
    {
    public:
        using class_type = SPIADCStarter<PO>;

        SPIADCStarter() {}
        SPIADCStarter(const class_type&) = delete;
        SPIADCStarter& operator=(const class_type&) = delete;
        SPIADCStarter(class_type&&) = delete;
        SPIADCStarter& operator=(class_type&&) = delete;

    private:
        uint8_t param_func() const
        {
            return 0;
        }

    public:

        bool start_adc() const noexcept
        {
            constexpr uint8_t start_adc_cmd = 1;
            return setup_command<PO>(this, &class_type::param_func, start_adc_cmd, "Start ADC command OK: ", (int) start_adc_cmd);
        }

        bool stop_adc() const  noexcept
        {
            constexpr uint8_t stop_adc_cmd = 0;
            return setup_command<PO>(this,  &class_type::param_func, stop_adc_cmd, "Stop ADC command OK: ", (int) stop_adc_cmd);
        }
    };

//    static size_t adc_buf_ptr = 0;
//    static size_t adc_buf_end = 0;
//
//    inline void fill_adc_buffer (uint32_t len, recv_sample_type * const adc_buf)
//    {
//        auto const sample_count = len / sizeof(recv_sample_type);
//        auto const rest = adc_buf_end - adc_buf_ptr;
//        if (rest > sample_count)
//        {
//            memcpy (&adc_buf[adc_buf_ptr], rcv_buf, len);
//            adc_buf_ptr += sample_count;
//        } else
//        {
//            memcpy (&adc_buf[adc_buf_ptr], rcv_buf, rest * sizeof(recv_sample_type));
//            auto const sample_count_minus_rest = sample_count - rest;
//            if (sample_count_minus_rest)
//            {
//                memcpy (&adc_buf[0], &rcv_buf[rest * sizeof(recv_sample_type)], sample_count_minus_rest * sizeof(recv_sample_type));
//            }
//            adc_buf_ptr = sample_count_minus_rest;
//        }
//    }

    template<typename PO>
    void io_func(recv_sample_type * const adc_buf, const PO * const pr_opts)
    {
        SPIADCStarter<PO> starter;
        if (!starter.start_adc())
        {
            Log_Wrapper("Не удалось запустить сбор данных!");
            return;
        }

        io_flag = true;

//        adc_buf_ptr = 0;
//        adc_buf_end = pr_opts->cfg_adc_.rate * pr_opts->cfg_adc_.adc_buf_size_in_seconds;
//        update_transfer_status(adc_buf_ptr, *pr_opts);

        while (io_flag)
        {
            if (auto const len = read_buffer<PO>(rcv_buf, read_ready_flag_command<PO>()))
            {
                static std::ofstream binary_file {"file.bin", std::ios::out | std::ios::binary};
                binary_file.write ((char*)rcv_buf, len);                
            }
//            {
//                fill_adc_buffer (len, adc_buf);
//                update_transfer_status(adc_buf_ptr, *pr_opts);
//            } else
//            {
//                platform_sleep_micro(100);
//            }
        }
        if (!starter.stop_adc())
        {
            Log_Wrapper("Can't stop data acquisition!");
        } else
        {
            Log_Wrapper("Data acquisition successfully stopped.");
        }
    }
    
}

#endif /* BOARD_THREAD_H */

