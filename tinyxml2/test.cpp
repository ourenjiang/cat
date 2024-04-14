#include <iostream>
#include "tinyxml2.h"
using namespace std;
using namespace tinyxml2;

void createXML(const char *path)
{
    //如果对声明头的格式有特殊要求，可以自定义生成
    // const char *decl = "<?xml version\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
    // XMLDocument doc;
    // doc.Parse(decl);

    XMLDocument doc;
    doc.InsertFirstChild(doc.NewDeclaration());

    //添加根元素
    doc.InsertEndChild(doc.NewElement("DBUSER"));

    //保存文件
    doc.SaveFile(path);
}

void insertXML(const char *path)
{
    XMLDocument doc;
    if(XML_SUCCESS != doc.LoadFile(path)){
        return;
    }

    //获取根结点
    XMLElement *root = doc.RootElement();

    //添加User元素
    XMLElement *user = doc.NewElement("User");
    user->SetAttribute("name", "kitty");//添加name属性，类型为char*，其他类型还包括基本数据类型、布尔类型
    user->SetAttribute("age", 18);
    root->InsertEndChild(user);

    //添加邮件
    XMLElement *email = doc.NewElement("Email");
    email->InsertEndChild(doc.NewText("1431244343@qq.com"));
    user->InsertEndChild(email);

    //添加Work元素
    XMLElement *work = doc.NewElement("Work");
    work->SetAttribute("salary", 9999);
    work->SetAttribute("location", "南山区");
    root->InsertEndChild(work);

    //保存文件
    doc.SaveFile(path);
}

void queryXML(const char *path)
{

    XMLDocument doc;
    if(XML_SUCCESS != doc.LoadFile(path)){
        return;
    }

    //获取声明
    XMLNode *node = doc.FirstChild();
    if(!node){
        return;
    }

    XMLDeclaration *decl = node->ToDeclaration();
    printf("declaration: %s\n", decl->Value());

    //获取根结点
    XMLElement *root = doc.RootElement();

    //解析User节点
    XMLElement *user = root->FirstChildElement("User");
    if(!user){
        return;
    }

    //获取属性值
    printf("User.name : %s\n", user->Attribute("name"));
    printf("User.age : %s\n", user->Attribute("age"));

    //解析email
    XMLElement *email = user->FirstChildElement("Email");
    if(!email){
        return;
    }

    //获取email内容
    printf("email: %s\n", email->GetText());

    //解析Work节点
    // XMLElement *work = root->FirstChildElement("Work");
    XMLElement *work = user->NextSiblingElement();
    if(!work){
        return;
    }

    //获取属性值
    printf("Work.salary : %s\n", work->Attribute("salary"));
    printf("Work.location : %s\n\n", work->Attribute("location"));
}

void modifyXML(const char *path)
{

    XMLDocument doc;
    if(XML_SUCCESS != doc.LoadFile(path)){
        return;
    }

    //获取根结点
    XMLElement *root = doc.RootElement();

    //解析User节点
    XMLElement *user = root->FirstChildElement("User");
    if(!user){
        return;
    }

    //修改属性值
    user->SetAttribute("name", "myname");
    user->SetAttribute("age", 99);

    //解析email
    XMLElement *email = user->FirstChildElement("Email");
    if(!email){
        return;
    }

    //修改email内容
    email->SetText("123@163.com");

    //解析Work节点
    // XMLElement *work = root->FirstChildElement("Work");
    XMLElement *work = user->NextSiblingElement();
    if(!work){
        return;
    }

    //修改属性值
    work->SetAttribute("salary", 888);
    work->SetAttribute("location", "龙岗区");

    //保存文件
    doc.SaveFile(path);
}

void deleteXML(const char *path)
{
    XMLDocument doc;
    if(XML_SUCCESS != doc.LoadFile(path)){
        return;
    }

    //获取根结点
    XMLElement *root = doc.RootElement();

    //解析User节点
    XMLElement *user = root->FirstChildElement("User");
    if(!user){
        return;
    }

    //删除属性值
    user->DeleteAttribute("name");
    //删除子元素
    // user->DeleteChild(user->FirstChildElement("Email"));

    //删除所有子元素
    root->DeleteChildren();

    //保存文件
    doc.SaveFile(path);
}

void printXML(const char *path)
{
    XMLDocument doc;
    if(XML_SUCCESS != doc.LoadFile(path)){
        return;
    }

    FILE *fp = fopen("create_copy.xml", "w+");
    XMLPrinter printer(fp);
    doc.Print(&printer);

    printer.OpenElement("DBUSER");//不是在现有数据中查找元素，而是创建新元素（即使现有内容中存在同名元素）。
	printer.PushAttribute( "foo", "bar" );
    printer.CloseElement();

}


int main(int argc, char *argv[])
{
    const char *path_create = "create.xml";
    createXML(path_create);
    insertXML(path_create);
    queryXML(path_create);
    // modifyXML(path_create);
    // queryXML(path_create);
    // deleteXML(path_create);
    // printXML(path_create);
    return 0;
}
