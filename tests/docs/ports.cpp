/**
 * @file ports.cpp
 * @brief Instantiates the driver over the documented UART port, so every member compiles against it.
 */
#include "my_uart.hpp"

template class fdo2::Driver<MyUart>;
