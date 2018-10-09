/*
  problemcollector.cpp

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Anton Kreuzkamp <anton.kreuzkamp@kdab.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  accordance with GammaRay Commercial License Agreement provided with the Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to you.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Own
#include "problemcollector.h"

#include "probe.h"

using namespace GammaRay;

ProblemCollector::ProblemCollector(QObject *parent)
    : QObject(parent)
    , m_runningScanCount(0)
    , m_reportFinishedTimer(new QTimer(this))
{
    connect(m_reportFinishedTimer, SIGNAL(timeout()), this, SLOT(maybeEmitScansFinished()));
    m_reportFinishedTimer->setSingleShot(true);
    m_reportFinishedTimer->setInterval(10);
}

ProblemCollector * ProblemCollector::instance()
{
    return Probe::instance()->problemCollector();
}

void GammaRay::ProblemCollector::requestScan()
{
    // Remove all elements which originate from a previous scan, before doing a new scan
    // and do so, properly informing the model about all changes.
    auto firstToDeleteIt = m_problems.begin();
    auto it = firstToDeleteIt;
    while (true) {
        if (it != m_problems.end() && it->findingCategory == Problem::Scan) {
            ++it;
        } else if (firstToDeleteIt != it) { // this is supposed to be called also if `it == m_problems.end()`
            auto firstRow = std::distance(m_problems.begin(), firstToDeleteIt);
            auto count = std::distance(m_problems.begin(), it) - firstRow;
            emit aboutToRemoveProblems(firstRow, count);
            firstToDeleteIt = it = m_problems.erase(firstToDeleteIt, it);
            emit problemsRemoved();
        } else if (it != m_problems.end()) {
            ++it;
            ++firstToDeleteIt;
        } else {
            break;
        }
    }

    emit problemScanRequested();
}

void ProblemCollector::addProblem(const Problem& problem)
{
    auto self = instance();

    if (self->m_problems.contains(problem))
        return;

    emit self->aboutToAddProblem(self->m_problems.size());
    self->m_problems.push_back(problem);
    emit self->problemAdded();
}
void ProblemCollector::removeProblem(const QString& problemId)
{
    auto self = instance();
    auto it = std::find_if(self->m_problems.begin(), self->m_problems.end(), [&](const Problem &problem) { return problem.problemId == problemId; });
    if (it == self->m_problems.end())
        return;
    auto row = std::distance(self->m_problems.begin(), it);

    emit self->aboutToRemoveProblems(row);
    self->m_problems.erase(it);
    emit self->problemsRemoved();
}

const QVector<Problem> & ProblemCollector::problems()
{
    return m_problems;
}

void GammaRay::ProblemCollector::reportScanStarted()
{
    auto self = instance();

    ++self->m_runningScanCount;
    self->m_reportFinishedTimer->stop();
}
void GammaRay::ProblemCollector::reportScanFinished()
{
    auto self = instance();
    Q_ASSERT(self->m_runningScanCount > 0);

    --self->m_runningScanCount;

    if (self->m_runningScanCount == 0) {
        // As lots of the problem collecting runs single-threaded and
        // synchronous, it's a common scenario that one scan starts and
        // finishes before another one has the chance to start. That's why we
        // defer sending a finished signal to the client by 10ms.
        self->m_reportFinishedTimer->start();
    }
}

void GammaRay::ProblemCollector::maybeEmitScansFinished()
{
    if (m_runningScanCount == 0) {
        emit problemScansFinished();
    }
}


