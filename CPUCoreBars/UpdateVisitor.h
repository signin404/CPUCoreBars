#pragma once

// ADDED: This header must import the assembly it depends on to resolve its base class.
#using "LibreHardwareMonitorLib.dll"

using namespace LibreHardwareMonitor::Hardware;

namespace HardwareMonitor
{
    public ref class UpdateVisitor : IVisitor
    {
    public:
        virtual void VisitComputer(IComputer^ computer);
        virtual void VisitHardware(IHardware^ hardware);
        virtual void VisitSensor(ISensor^ sensor);
        virtual void VisitParameter(IParameter^ parameter);
    };
}