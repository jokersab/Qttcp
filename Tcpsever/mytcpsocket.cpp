#include "mytcpsocket.h"
#include<QDebug>
#include "opdb.h"
#include <QMessageBox>
#include "mytcpserver.h"
#include <QDir>
#include <QFileInfoList>

MyTcpSocket::MyTcpSocket(QObject *parent)
    : QTcpSocket{parent}
{
    connect(this,SIGNAL(readyRead()),this,SLOT(recvmsg()));
    connect(this,SIGNAL(disconnected()),this,SLOT(clientoffline()));
}

QString MyTcpSocket::getname()
{
    return m_name;
}

void MyTcpSocket::handregist(pdu *Pdu)
{
    char name[32] = {'\0'};
    char pwd[32] = {'\0'};
    strncpy(name,Pdu->caData,32);
    strncpy(pwd,Pdu->caData+32,32);
    bool ret = opDB::getinstance().handregist(name,pwd);
    pdu *respon= mkpud(0);
    respon->uiMsgType = msg_type_regist_respond;
    if(ret){
        strcpy(respon->caData,REGIST_OK);
        QDir dir;
    }else{
        strcpy(respon->caData,REGIST_FAILED);
    }
    write((char*)respon,respon->uiPDUlen);
    free(respon);
    respon = NULL;
}

void MyTcpSocket::handlogin(pdu *Pdu)
{
    char name[32] = {'\0'};
    char pwd[32] = {'\0'};
    strncpy(name,Pdu->caData,32);
    strncpy(pwd,Pdu->caData+32,32);
    bool ret = opDB::getinstance().handlogin(name,pwd);
    pdu *respon= mkpud(0);
    respon->uiMsgType = msg_type_login_respond;
    if(ret){
        strcpy(respon->caData,LOGIN_OK);
        m_name = name;
        QDir dir;
        dir.mkdir(QString("./%1").arg(name));
    }else{
        strcpy(respon->caData,LOGIN_FAILED);
    }
    write((char*)respon,respon->uiPDUlen);
    free(respon);
    respon = NULL;
}

void MyTcpSocket::handoline()
{
    QStringList ret = opDB::getinstance().handonline();
    uint len = ret.size()*32;
    pdu *Pdu1 = mkpud(len);
    Pdu1->uiMsgType = msg_type_online_respond;
    for(int i = 0; i < ret.size(); ++i){
        memcpy((char*)(Pdu1->caMsg)+i*32,ret[i].toStdString().c_str(),ret[i].size());
    }
    write((char*)Pdu1,Pdu1->uiPDUlen);
    free(Pdu1);
    Pdu1 = NULL;
}

void MyTcpSocket::handsearch(pdu *Pdu)
{
    int ret = opDB::getinstance().handsearch(Pdu->caData);
    pdu *respon = mkpud(0);
    respon->uiMsgType = msg_type_search_respond;
    if(ret == -1){
        strcpy(respon->caData,SEARCH_USER_EMPTY);
    }else if(ret == 1){
        strcpy(respon->caData,SEARCH_USER_OK);
    }else{
        strcpy(respon->caData,SEARCH_USER_OFFLINE);
    }
    write((char*)respon,respon->uiPDUlen);
    free(respon);
    respon = NULL;
}

void MyTcpSocket::handaddfriend(pdu *Pdu)
{
    char pname[32] = {'\0'};
    char mname[32] = {'\0'};
    strncpy(pname,Pdu->caData,32);
    strncpy(mname,Pdu->caData+32,32);
    int ret = opDB::getinstance().handadd(pname,mname);
    pdu *respon = mkpud(0);
    respon->uiMsgType = msg_type_addfri_respond;
    if(ret == -1){
        strcpy(respon->caData,UNKNOW_ERROR);
    }else if(0 == ret){
        strcpy(respon->caData,EXEITED_FRIEND);
    }else if(1 == ret){
        mytcpserver::getinstace().transend(pname,Pdu);
    }else if(2 == ret){
        strcpy(respon->caData,ADD_FRIEND_OFFLINE);
    }else if(3 == ret){
        strcpy(respon->caData,ADD_FRIEND_NO_EXIT);
    }
    write((char*)respon,respon->uiPDUlen);
    free(respon);
    respon = NULL;
}

void MyTcpSocket::handagreeadd(pdu *Pdu)
{
    char addedName[32] = {'\0'};
    char sourceName[32] = {'\0'};
    // 拷贝读取的信息
    strncpy(addedName, Pdu -> caData, 32);
    strncpy(sourceName, Pdu -> caData + 32, 32);
    opDB::getinstance().handagreeadd(addedName,sourceName);
    mytcpserver::getinstace().transend(sourceName,Pdu);
}

void MyTcpSocket::handflush(pdu *Pdu)
{
    char cname[32] = {'\0'};
    strncpy(cname,Pdu->caData,32);
    QStringList ans = opDB::getinstance().handflush(cname);
    uint uimsglen = ans.size()*32;
    pdu *respon = mkpud(uimsglen);
    for(int i = 0; i < ans.size(); ++i){
        memcpy((char *)(respon->caMsg)+i*32,ans.at(i).toStdString().c_str(),ans.at(i).size());
    }
    respon->uiMsgType = msg_flush_respon;
    write((char*)respon,respon->uiPDUlen);
    free(respon);
    respon = NULL;
}

void MyTcpSocket::handdelfriend(pdu *Pdu)
{
    char mName[32] = {'\0'};
    char fName[32] = {'\0'};
    // 拷贝读取的信息
    strncpy(mName, Pdu -> caData, 32);
    strncpy(fName, Pdu -> caData + 32, 32);
    opDB::getinstance().handdelfriend(mName,fName);
    pdu *respon = mkpud(0);
    respon->uiMsgType = msg_delfriend_respon;
    strcpy(respon->caData,DEL_FRIEND_OK);
    write((char*)respon,respon->uiPDUlen);
    mytcpserver::getinstace().transend(fName,Pdu);
    free(respon);
    respon = NULL;
}

void MyTcpSocket::handprivatechat(pdu *Pdu)
{
    char fname[32] = {'\0'};
    memcpy(fname,Pdu->caData+32,32);
    QString str = QString::fromLocal8Bit(fname);
    QStringList list = str.split(" ");
    QString newfname = list[0];
    strcpy(fname,newfname.toStdString().c_str());
    mytcpserver::getinstace().transend(fname,Pdu);
    qDebug()<<fname;
}

void MyTcpSocket::handgroupchat(pdu *Pdu)
{
    char caname[32] = {'\0'};
    strncpy(caname,Pdu->caData,32);
    QStringList online = opDB::getinstance().handflush(caname);
    QString tmp;
    for(int i = 0; i < online.size(); ++i){
        tmp = online[i].split(" ").at(0);
        mytcpserver::getinstace().transend(tmp.toStdString().c_str(),Pdu);
    }
}

void MyTcpSocket::handcreatdir(pdu *Pdu)
{
    QDir dir;
    QString curpath = QString("%1").arg((char*)Pdu->caMsg);
    pdu *respon = mkpud(0);
    respon->uiMsgType = msg_creat_dir_respon;
    bool ret = dir.exists(curpath);     //文件夹是否存在
    if(ret){
        char creatpath[32];
        memcpy(creatpath,Pdu->caData+32,32);
        QString newpath = curpath + "/" + creatpath;
        ret = dir.exists(newpath);
        if(ret){    //新路径是否存在
            strcpy(respon->caData,FILE_NAME_EXITED);
        }else{
            dir.mkdir(newpath);
            strcpy(respon->caData,CREATE_DIR_OK);
        }
    }else{
        strcpy(respon->caData,DIR_NO_EXIT);
    }
    write((char*)respon,respon->uiPDUlen);
    free(respon);
    respon = NULL;
}

void MyTcpSocket::handflushfile(pdu *Pdu)
{
    char *curpath = new char[Pdu->uiMsgLen];
    memcpy(curpath,Pdu->caMsg,Pdu->uiMsgLen);
    QDir dir(curpath);
    QFileInfoList filelist = dir.entryInfoList();
    int filecount = filelist.size();
    pdu *respon = mkpud(sizeof(fileinfo)*(filecount-2));
    respon->uiMsgType = msg_flushfile_respon;
    fileinfo *tmpinfo = NULL;
    QString tmpfilename;
    for(int i = 2; i < filecount; ++i){
        tmpinfo = (fileinfo*)(respon->caMsg)+i-2;
        tmpfilename = filelist[i].fileName();
        memcpy(tmpinfo->filename,tmpfilename.toStdString().c_str(),tmpfilename.size());
        if(filelist[i].isDir()){
            tmpinfo->filetype = 0;
        }else if(filelist[i].isFile()){
            tmpinfo->filetype = 1;
        }
    }
    write((char*)respon,respon->uiPDUlen);
    free(respon);
    respon = NULL;
}

void MyTcpSocket::recvmsg()
{
    uint uiPduLen = 0;
    this->read((char*)&uiPduLen,sizeof(uint));
    uint uiMsgLen = uiPduLen - sizeof(pdu);
    pdu *Pdu = mkpud(uiMsgLen);
    this->read((char*)Pdu + sizeof(uint),uiPduLen - sizeof(uint));
    switch (Pdu->uiMsgType) {
    case msg_type_regist_request:
    {
        handregist(Pdu);
        break;
    }
    case msg_type_login_request:
    {
        handlogin(Pdu);
        break;
    }
    case msg_type_online_request:{
        handoline();
        break;
    }
    case msg_type_search_request:{
        handsearch(Pdu);
        break;
    }
    case msg_type_addfri_request:{
        handaddfriend(Pdu);
        break;
    }
    case msg_type_agree_add_friend:{
        handagreeadd(Pdu);
        break;
    }
    case msg_typr_disagree_add:{
        char sourceName[32] = {'\0'};
        // 拷贝读取的信息
        strncpy(sourceName, Pdu -> caData + 32, 32);
        // 服务器需要转发给发送好友请求方其被拒绝的消息
        mytcpserver::getinstace().transend(sourceName,Pdu);
        break;
    }
    case msg_flush_request:{
        handflush(Pdu);
        break;
    }
    case msg_delfriend_request:{
        handdelfriend(Pdu);
        break;
    }
    case msg_privatechat_request:{
        handprivatechat(Pdu);
        break;
    }
    case msg_group_chat_request:{
        handgroupchat(Pdu);
        break;
    }
    case msg_creat_dir_request:{
        handcreatdir(Pdu);
        break;
    }
    case msg_flushfile_request:{
        handflushfile(Pdu);
        break;
    }
    default:
        break;
    }
    // qDebug()<<name<<pwd;
}

void MyTcpSocket::clientoffline()
{
    opDB::getinstance().handoffline(m_name.toStdString().c_str());
    emit offline(this);

}
