#pragma once

#include<map>
#include<string>
#include<stdlib.h>
#include<mysql/mysql.h>
#include<algorithm>
#include<stdint.h>
#include"common/logging.hpp"

namespace dan
{

class danMysql
{
public:
    danMysql();
    ~danMysql();

    void disconnect();//关闭和mysql的连接
    bool connect(const char* ip,
                 const char* userName,
                 const char* userPasswd,
                 const uint16_t port=3306,
                 const char* dbName=NULL);//连接到mysql
    const std::string escapeString(const std::string&value)const;
    MYSQL*getMysql(){return mysql_;}
private:
    MYSQL*mysql_;
    bool isConnected_;
    const char* ip_;
    const char* userName_;
    const char* userPasswd_;
    const char* dbName_;
    uint16_t port_; 
};

class danMysqlDo
{
public:
    danMysqlDo(danMysql*ptr,const std::string& sql);
    ~danMysqlDo(){}
    bool setString(const uint8_t& idx,const std::string& value);
    bool setInt(const uint8_t& idx,const int& value);
    bool setDouble(const uint8_t& idx,const double& value); 
    bool setNull(const uint8_t& idx);

    uint32_t getRowNum(){return rowValue_.size();}
    uint32_t getFieldNum(){return fieldName_.size();}
    const std::string makeSql();//填入参数,完成sql语句

    const std::string getFieldValue(uint32_t row , uint32_t field);
    int doInsert();//执行插入
    bool doQuery();//执行查询
private:
    typedef std::map<uint32_t,std::string>fieldValue_t;//域值
    std::map<uint32_t,fieldValue_t> rowValue_;//行值
    std::map<uint32_t,std::string> args_;  //参数
    std::map<uint32_t,std::string> fieldName_; //域名字
    danMysql* danMysqlPtr_;
    std::string sql_;
    uint8_t argsCount_; //参数个数
};

//===============================================
const std::string danMysqlDo::getFieldValue(uint32_t row,uint32_t field)
{
    if(row>=getRowNum())
    {
        DAN_LOG_ERROR<<"getRowNum 错误";
        return "";
    }
    if(field>=getFieldNum())
    {
        DAN_LOG_ERROR<<"getFieldNum 错误";
        return "";
    }
    return (rowValue_[row])[field];
}
bool danMysqlDo::doQuery()
{
    std::string sql=makeSql();
    if(mysql_query(danMysqlPtr_->getMysql(),sql.c_str()))
    {
        DAN_LOG_ERROR<<"doQuery 错误:"<<mysql_error(danMysqlPtr_->getMysql());
        return false;
    }
    MYSQL_RES* result=mysql_store_result(danMysqlPtr_->getMysql());
    if(result==NULL)
    {
        //发生错误
        DAN_LOG_ERROR<<"doQuery 错误:"<<mysql_error(danMysqlPtr_->getMysql());
        mysql_free_result(result);
        return false;
    }

    uint32_t fieldNum=mysql_num_fields(result);
    MYSQL_ROW row;
    MYSQL_FIELD*field;

    uint32_t i=0;
    while((field=mysql_fetch_field(result)))
    {
        fieldName_[i]=field->name;
        ++i;
    }
    i=0;
    while((row=mysql_fetch_row(result)))
    {
        fieldValue_t fieldValue;
        for(uint32_t n=0;n<fieldNum;n++)
        {
            //填充每个域值
            fieldValue[n]=(row[n]? row[n]:"NULL");
           // std::cout<<fieldValue[n]<<std::endl;
        }
        //填充每个行值
        rowValue_[i]=fieldValue;
        i++;
    }
    mysql_free_result(result);
    //std::cout<<(rowValue_[0])[0];
    return true;
}

const std::string danMysqlDo::makeSql()
{
    int foundPos=0;
    int beginPos=0;
    std::string sql;
    sql=sql_;
    for(unsigned int i=0;i<argsCount_;++i)
    {
        foundPos=sql.find('?',beginPos+foundPos);
        sql.replace(foundPos,1,args_[i]);
    }
    return sql;
}

int danMysqlDo::doInsert()
{
    std::string sql=makeSql();
    if(mysql_query(danMysqlPtr_->getMysql(),sql.c_str()))
    {
        DAN_LOG_ERROR<<"doInsert 错误:"<<mysql_error(danMysqlPtr_->getMysql());
        return -1;
    }
    int lastInsertId=mysql_insert_id(danMysqlPtr_->getMysql());
    return lastInsertId;
}
bool danMysqlDo::setString(const uint8_t& idx,const std::string& value)
{
    if(idx>argsCount_)
    {
        DAN_LOG_ERROR<<"参数个数不一致";
        return false;
    }
    std::stringstream ss;
    std::string escapedValue=danMysqlPtr_->escapeString(value);
    ss<<"\""<<escapedValue<<"\"";
    args_[idx]=ss.str();
    return true;

}

bool danMysqlDo::setInt(const uint8_t& idx,const int& value)
{
    if(idx>argsCount_)
    {
        DAN_LOG_ERROR<<"参数个数不一致";
        return false;
    }
    std::stringstream ss;
    ss<<value;
    args_[idx]=ss.str();
    return true;
}

bool danMysqlDo::setDouble(const uint8_t& idx,const double& value)
{
    if(idx>argsCount_)
    {
        DAN_LOG_ERROR<<"参数个数不一致";
        return false;
    }
    std::stringstream ss;
    ss<<value;
    args_[idx]=ss.str();
    return true;
}

bool danMysqlDo::setNull(const uint8_t& idx)
{
    if(idx>argsCount_)
    {
        DAN_LOG_ERROR<<"参数个数不一致";
        return false;
    }
    args_[idx]="NULL";
    return true;
}
danMysqlDo::danMysqlDo(danMysql*ptr,const std::string& sql):
    danMysqlPtr_(ptr),
    sql_(sql),
    argsCount_(0)
{
    if(!sql.empty())
        argsCount_=std::count(sql.begin(),sql.end(),'?');
    //std::cout<<argsCount_;
}

//===============================================
const std::string danMysql::escapeString(const std::string&value)const
{
    if(!isConnected_)
    {
        DAN_LOG_ERROR<<"未连接到mysql，无法转义字符串";
        return "";
    }
    char*cValue=(char*)calloc(1,value.length()*2+1);
    mysql_real_escape_string(mysql_,cValue,value.c_str(),value.length());
    std::string ret=cValue;
    free (cValue);
    return ret;
}
bool danMysql::connect(const char* ip,
                 const char* userName,
                 const char* userPasswd,
                 const uint16_t port,
                 const char* dbName)
{
    disconnect(); //先断开之前的连接

    ip_=ip;
    userName_=userName;
    userPasswd_=userPasswd;
    port_=port;
    mysql_=mysql_init(NULL); //新建一个
    if(mysql_real_connect(mysql_,
                          ip,
                          userName,
                          userPasswd,
                          dbName,
                          port,NULL,0)==NULL)
    {
        //连接失败
        isConnected_=false;
        DAN_LOG_ERROR<<"连接mysql失败:"<<mysql_error(mysql_);
        return false;
    }
    else
    {
        //连接成功
        isConnected_=true;
        DAN_LOG_WARNING<<"连接mysql成功";
        return true;
    }


}

void danMysql::disconnect()
{
    if(mysql_==nullptr)
        return;
    mysql_close(mysql_);
    isConnected_=false;
}

danMysql::danMysql():
    mysql_(nullptr),isConnected_(false),port_(0)
{
}

danMysql::~danMysql()
{
    DAN_LOG_WARNING<<"关闭和mysql的连接";
    disconnect();
}





}
