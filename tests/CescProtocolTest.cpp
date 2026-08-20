#include "services/cesc/CescPacketCodec.h"
#include "services/cesc/CescTransactionManager.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <cassert>
#include <cmath>

static QByteArray hex(const char *s){ return QByteArray::fromHex(s); }
int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    Cesc::Packet empty{1,Cesc::MessageType::Request,0,2,1,{}};
    assert(CescPacketCodec::encode(empty)==hex("43450100000201000000E475"));
    Cesc::Packet ping{1,Cesc::MessageType::Request,0,2,0x1234,hex("78563412")};
    const QByteArray vector=CescPacketCodec::encode(ping);
    assert(vector==hex("4345010000023412040078563412600D"));
    for(int split=0;split<=vector.size();++split){ CescPacketCodec c; assert(c.feed(vector.left(split)).isEmpty() || split==vector.size()); auto p=c.feed(vector.mid(split)); if(split<vector.size()) assert(p.size()==1 && p[0]==ping); }
    CescPacketCodec one; QList<Cesc::Packet> got; for(char b:vector) got+=one.feed(QByteArray(1,b)); assert(got.size()==1);
    CescPacketCodec multi; assert(multi.feed(vector+vector).size()==2);
    CescPacketCodec garbage; assert(garbage.feed(hex("004343004345")+vector).size()==1);
    for(int i=2;i<vector.size()-2;++i){ QByteArray bad=vector; bad[i]^=1; CescPacketCodec c; const auto decoded=c.feed(bad+QByteArray(5000,'x')+vector); assert(decoded.size()==1&&decoded[0]==ping); }
    QByteArray oversized=hex("43450100000201000110"); CescPacketCodec small(16); assert(small.feed(oversized).isEmpty()); assert(small.diagnostics().lengthErrors==1);
    QByteArray values; Cesc::appendU16(values,0x1234); Cesc::appendU32(values,0x89abcdef); Cesc::appendU64(values,0x0123456789abcdefULL); Cesc::appendFloat32(values,1.25f); Cesc::appendFloat64(values,-2.5);
    qsizetype p=0; quint16 u16; quint32 u32; quint64 u64; float f; double d; assert(Cesc::readU16(values,p,u16)&&u16==0x1234); assert(Cesc::readU32(values,p,u32)&&u32==0x89abcdef); assert(Cesc::readU64(values,p,u64)&&u64==0x0123456789abcdefULL); assert(Cesc::readFloat32(values,p,f)&&std::fabs(f-1.25f)<1e-6); assert(Cesc::readFloat64(values,p,d)&&std::fabs(d+2.5)<1e-12);
    CescTransactionManager tx; QList<Cesc::Packet> sent; QObject::connect(&tx,&CescTransactionManager::sendPacket,[&](const Cesc::Packet &v){sent<<v;});
    const quint16 a=tx.request(0,1,{},100,0), b=tx.request(0,2,{},100,0); assert(a&&b&&a!=b); QByteArray ok; Cesc::appendU16(ok,0); assert(tx.handleResponse({1,Cesc::MessageType::Response,0,2,b,ok})); assert(tx.handleResponse({1,Cesc::MessageType::Response,0,1,a,ok}));
    CescTransactionManager retry; QList<Cesc::Packet> attempts; QObject::connect(&retry,&CescTransactionManager::sendPacket,[&](const Cesc::Packet &v){attempts<<v;}); retry.request(1,1,"same",1,1); QEventLoop loop; QTimer::singleShot(20,&loop,&QEventLoop::quit); loop.exec(); assert(attempts.size()==2&&attempts[0]==attempts[1]);
    return 0;
}
