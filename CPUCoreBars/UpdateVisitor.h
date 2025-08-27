#pragma once

// This header must import the assembly it depends on to define its base class (IVisitor).
#using "LibreHardwareMonitorLib.dll"

using namespace LibreHardwareMonitor::Hardware;

namespace HardwareMonitor
{
    // This is a C++/CLI managed reference class that implements the IVisitor interface.
    public ref class UpdateVisitor : IVisitor
    {
    public:
        // These are the methods required by the IVisitor interface.
        virtual void VisitComputer(IComputer^ computer);
        virtual void VisitHardware(IHardware^ hardware);
        virtual void VisitSensor(ISensor^ sensor);
        virtual void VisitParameter(IParameter^ parameter);
    };
}