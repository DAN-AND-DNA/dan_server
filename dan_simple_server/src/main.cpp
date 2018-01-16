#include"dan_net.hpp"
#include"dan.hpp"
int main()
{
    //TODO  测试网络
    dan::danNet p;
    p.run();
/*
    lulu::ask_me a,b;
    std::cout<<a.ByteSize()<<std::endl;
    a.set_id(7);
    a.add_name("dan");
    std::cout<<a.ByteSize()<<std::endl;
    a.add_name("yang");
    std::cout<<a.ByteSize()<<std::endl;
    uint8_t*data=(uint8_t*)calloc(a.ByteSize(),sizeof(uint8_t));
    uint8_t*data1=(uint8_t*)calloc(128,sizeof(uint8_t))
    a.

    if(a.SerializeToArray(data,a.ByteSize()))
        std::cout<<"s is ok\n";
    if(b.ParseFromArray(data,a.ByteSize()))
        std::cout<<"p is ok\n";

    std::cout<<b.name(1);
*/
    //TODO 1
/*    
    std::string type_name=lulu::ask_me::descriptor()->full_name();
    std::cout<<type_name<<std::endl;
    uint16_t id=dan::A::name_to_id(type_name);
    std::cout<<id<<std::endl;

    // TODO 2
    type_name=dan::A::id_to_name(id);
    const google::protobuf::Descriptor* d=google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
    std::cout<<d<<std::endl;
    std::cout<<lulu::ask_me::descriptor()<<std::endl;
    
    const google::protobuf::Message* p=google::protobuf::MessageFactory::generated_factory()->GetPrototype(d);

    std::cout<<p<<std::endl;
    std::cout<<&lulu::ask_me::default_instance()<<std::endl;

    //TODO 3
    lulu::ask_me* a=dynamic_cast<lulu::ask_me*>(p->New());
    
    a->set_id(7);
    a->add_name("dan");
    a->add_name("yang");
    uint8_t data[128];
    a->SerializeToArray(data,128);

    lulu::ask_me b;
    b.ParseFromArray(data,128);
    std::cout<<b.id()<<std::endl;
    std::cout<<b.name(0)<<std::endl;
    std::cout<<b.name(1)<<std::endl;

    delete a;
    */
}
