#include "boost/asio.hpp"
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include "boost/bind.hpp"

#include <iostream>
#include <stdlib.h>

using namespace std;

void writeHandler(const boost::system::error_code& error, size_t bytesTransferred)
{
    if(!error)
    {
        cout << "Data send Successfully." << endl;
    }
    else
    {
        cerr << "Error sending data: " << error.message() << endl;
    }
}

int main(int argc, char **argv)
{
    boost::asio::io_context io;

    boost::asio::serial_port serial(io);
    serial.open(argv[1]);

    serial.set_option(boost::asio::serial_port_base::baud_rate(9600));
    serial.set_option(boost::asio::serial_port_base::character_size(8));
    serial.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port::stop_bits::one));
    serial.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port::parity::none));

    string data_to_send = "Hello World";

    //异步写入
    boost::asio::async_write(serial, boost::asio::buffer(data_to_send),
        boost::bind(writeHandler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

    io.run();
}
