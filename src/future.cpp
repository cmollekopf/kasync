/*
 * Copyright 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "future.h"
#include "async.h"

using namespace KAsync;

QDebug &operator<<(QDebug &dbg, const Error &error)
{
    dbg << "Error: " << error.errorCode << "Msg: " << error.errorMessage;
    return dbg;
}

FutureBase::PrivateBase::PrivateBase(const Private::ExecutionPtr &execution)
    : finished(false)
    , mExecution(execution)
{
}

FutureBase::PrivateBase::~PrivateBase()
{
    Private::ExecutionPtr executionPtr = mExecution.toStrongRef();
    if (executionPtr) {
        executionPtr->releaseFuture();
        releaseExecution();
    }
}

void FutureBase::PrivateBase::releaseExecution()
{
    mExecution.clear();
}



FutureBase::FutureBase()
    : d(nullptr)
{
}

FutureBase::FutureBase(FutureBase::PrivateBase *dd)
    : d(dd)
{
}

FutureBase::FutureBase(const KAsync::FutureBase &other)
    : d(other.d)
{
}

FutureBase::~FutureBase()
{
}

void FutureBase::releaseExecution()
{
    d->releaseExecution();
}

void FutureBase::setFinished()
{
    if (isFinished()) {
        return;
    }
    d->finished = true;
    //TODO this could directly call the next continuation with the value, and thus avoid unnecessary copying.
    for (auto watcher : d->watchers) {
        if (watcher) {
            watcher->futureReadyCallback();
        }
    }
}

bool FutureBase::isFinished() const
{
    return d->finished;
}

void FutureBase::setError(int code, const QString &message)
{
    d->errors.clear();
    addError(Error(code, message));
    setFinished();
}

void FutureBase::setError(const Error &error)
{
    d->errors.clear();
    addError(error);
    setFinished();
}

void FutureBase::addError(const Error &error)
{
    d->errors << error;
}

void FutureBase::clearErrors()
{
    d->errors.clear();
}

bool FutureBase::hasError() const
{
    return !d->errors.isEmpty();
}

int FutureBase::errorCode() const
{
    if (d->errors.isEmpty()) {
        return 0;
    }
    return d->errors.first().errorCode;
}

QString FutureBase::errorMessage() const
{
    if (d->errors.isEmpty()) {
        return QString();
    }
    return d->errors.first().errorMessage;
}

QVector<Error> FutureBase::errors() const
{
    return d->errors;
}

void FutureBase::setProgress(int processed, int total)
{
    setProgress((qreal) processed / total);
}

void FutureBase::setProgress(qreal progress)
{
    for (auto watcher : d->watchers) {
        if (watcher) {
            watcher->futureProgressCallback(progress);
        }
    }
}



void FutureBase::addWatcher(FutureWatcherBase* watcher)
{
    d->watchers.append(QPointer<FutureWatcherBase>(watcher));
}





FutureWatcherBase::FutureWatcherBase(QObject *parent)
    : QObject(parent)
    , d(new FutureWatcherBase::Private)
{
}

FutureWatcherBase::~FutureWatcherBase()
{
    delete d;
}

void FutureWatcherBase::futureReadyCallback()
{
    Q_EMIT futureReady();
}

void FutureWatcherBase::futureProgressCallback(qreal progress)
{
    Q_EMIT futureProgress(progress);
}

void FutureWatcherBase::setFutureImpl(const FutureBase &future)
{
    d->future = future;
    d->future.addWatcher(this);
    if (future.isFinished()) {
        futureReadyCallback();
    }
}
