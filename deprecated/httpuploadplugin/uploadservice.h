/*
 * uploadservice.h
 *
 *  Created on: 24 Sep 2016
 *      Author: rkfg
 */

#ifndef UPLOADSERVICE_H
#define UPLOADSERVICE_H

#include <QString>

class UploadService {
public:
    UploadService(const QString& serviceName, int sizeLimit);

    const QString& serviceName() const {
        return serviceName_;
    }

    int sizeLimit() const {
        return sizeLimit_;
    }

private:
    QString serviceName_;
    int sizeLimit_;
};

#endif // UPLOADSERVICE_H
