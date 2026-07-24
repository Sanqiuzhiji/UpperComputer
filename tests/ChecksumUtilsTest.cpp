#include "utils/ChecksumUtils.h"

#include <QByteArray>

int main()
{
    const QByteArray standardVector("123456789");
    if (ChecksumUtils::crc8(standardVector) != 0xF4U) return 1;
    if (ChecksumUtils::crc8Maxim(standardVector) != 0xA1U) return 2;
    if (ChecksumUtils::xor8(QByteArray::fromHex("010203")) != 0x00U) return 3;
    return 0;
}
