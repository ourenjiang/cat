#include <iostream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class Person {
public:
    std::string name;
    int age;

    // 使用宏声明 getter 和 setter
    BOOST_PROPERTY_TREE_GETTER_SETTER(std::string, name, &Person::getName, &Person::setName)
    BOOST_PROPERTY_TREE_GETTER_SETTER(int, age, &Person::getAge, &Person::setAge)
};

int main() {
    Person person;
    person.setName("John");
    person.setAge(25);

    // 将对象转换为 Boost.PropertyTree
    boost::property_tree::ptree pt;
    pt.put("name", person.getName());
    pt.put("age", person.getAge());

    // 将 Boost.PropertyTree 转换为 JSON 字符串
    std::stringstream ss;
    boost::property_tree::write_json(ss, pt);
    std::cout << ss.str() << std::endl;

    return 0;
}
