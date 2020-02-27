/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   board_functions.hpp
 * Author: user
 *
 * Created on 27 февраля 2020 г., 14:48
 */

#ifndef BOARD_FUNCTIONS_HPP
#define BOARD_FUNCTIONS_HPP

namespace spi_adc_client
{
    template<typename PO, typename O, typename F, typename ... T>
    static bool setup_command(O const * const o, F const & fun, uint8_t cmd_number, T &&... args) noexcept
    {
        static_assert(std::is_same<uint8_t, typename std::result_of<F(O)>::type>::value, "");

        static uint8_t tx1[1] = {cmd_number};
        static typename std::result_of<F(O)>::type tx2[2] = {(o->*fun)(), 0};
        // альтернативно:
        // static uint8_t tx2[2] = {fun (o), 0};
        // в случае если fun : std::function<uint8_t(O const * const)> const & fun, а typename F отсутствует в списке параметров шаблона
        static uint8_t rx1[1], rx2[2];

        using myBoard = Loki::SingletonHolder<niipa::Board<PO>, Loki::CreateUsingNew>;

        static struct spi_ioc_transfer tr[2] {
            {(__u64)tx1, (__u64)rx1, sizeof(tx1), myBoard::Instance().get_speed(), 500, myBoard::Instance().get_bits(), 0,   0, 0, 0},
            {(__u64)tx2, (__u64)rx2, sizeof(tx2), myBoard::Instance().get_speed(), 100,  myBoard::Instance().get_bits(), 0,   0, 0, 0}
        };

        if (ioctl(myBoard::Instance().get_handle(), SPI_IOC_MESSAGE(2), tr) < 1)
        {
            Log_Wrapper ("Не удается выполнить системный вызов SPI_IOC_MESSAGE(2) для команды: ", (int)cmd_number);
            return false;
        }
        if (((rx2[1] << 8) | rx2[0]) != cmd_number)
        {
            Log_Wrapper ("Нет ответа на передачу команды на исполнение или ответ неверен, номер команды: ", (int)cmd_number);
            return false;
        } else {
            Log_Wrapper (std::forward<T>(args) ...);
        }
        return true;
    }
    
}


#endif /* BOARD_FUNCTIONS_HPP */

