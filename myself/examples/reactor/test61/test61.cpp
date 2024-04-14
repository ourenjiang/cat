#include "boost/asio.hpp"
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include "boost/bind.hpp"

#include <iostream>
#include <stdlib.h>

using namespace std;

void readHandler(const boost::system::error_code& error, size_t bytesTransferred, boost::asio::serial_port& serial, vector<char>& buffer)
{
    if(!error)
    {
        cout << "Received data: " << endl;
        // cout << serial.
        cout.write(buffer.data(), bytesTransferred);

        boost::asio::async_read(serial, boost::asio::buffer(buffer),
            boost::bind(readHandler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, boost::ref(serial), boost::ref(buffer)));
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

    vector<char> buffer(1024);

    //异步读取
    boost::asio::async_read(serial, boost::asio::buffer(buffer),
        boost::bind(readHandler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, boost::ref(serial), boost::ref(buffer)));

    io.run();
}
