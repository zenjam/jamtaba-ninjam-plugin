#include "UploadIntervalData.h"
#include <QUuid>

UploadIntervalData::UploadIntervalData() :
    GUID(newGUID())
{
}

UploadIntervalData::~UploadIntervalData()
{
    //
}

void UploadIntervalData::appendData(const QByteArray &encodedData)
{
    dataToUpload.append(encodedData);
}

ninjam::MessageGuid UploadIntervalData::newGUID()
{
    QUuid uuid = QUuid::createUuid();
    return ninjam::MessageGuid(uuid.toRfc4122());
}
