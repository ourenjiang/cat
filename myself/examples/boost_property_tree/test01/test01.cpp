#include <iostream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using namespace std;
using namespace boost::property_tree;

void CreateConfig(string filename)
{
	ptree pt;
	read_xml(filename, pt);

	pt.put("app.version", "1.0.0.1");
	pt.put("app.theme", "blue");
	pt.put("app.about.url", "http://www.xyz.com");
	pt.put("app.about.content", "coryright(C) xyz.com 2000-2010");

	write_xml(filename, pt);
}

int main()
{
	CreateConfig("config.xml");
}
