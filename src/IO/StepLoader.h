#ifndef STEPLOADER_H
#define STEPLOADER_H

#include <QString>
#include <TopoDS_Shape.hxx>

namespace OccQtCore
{
    struct StepLoadResult
    {
        bool success = false;
        TopoDS_Shape shape;
        QString errorMessage;
    };

    class StepLoader
    {
    public:
        static StepLoadResult load(const QString& filePath);
    };
}

#endif // STEPLOADER_H
