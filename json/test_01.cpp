/*
{
  "pi": 3.141,
  "happy": true,
  "name": "Boost",
  "nothing": null,
  "answer": {
    "everything": 42
  },
  "list": [1, 0, 2],
  "object": {
    "currency": "USD",
    "value": 42.99
  }
}*/

#include <iostream>

// #include <boost/json/src.hpp>
#include <boost/json.hpp>
using namespace boost::json;

int main()
{
    {
        // array, object, string
        // value

        object obj;
        obj["pi"] = 3.141;
    }

    {
        value jv = parse("[1, 2, 3]");

        error_code ec;
        value jv2 = parse(R"("Hello, world!")", ec);
    }

    {
        value jv = {1, 2, 3};
        std::string s = serialize(jv);
        // std::cout << s << std::endl;
    }

    {
        value jv = {1, 2, 3};
        serializer sr;
        sr.reset(&jv);

        do{
            char buf[1];
            std::cout << sr.read(buf) << std::endl;
        }
        while(!sr.done());
    }

    getchar();
}
