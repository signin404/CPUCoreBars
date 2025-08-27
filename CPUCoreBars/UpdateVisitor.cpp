#include "stdafx.h"
#include "UpdateVisitor.h"

namespace HardwareMonitor
{
    void UpdateVisitor::VisitComputer(IComputer^ computer)
    {
        // This tells the computer object to traverse all its hardware and sensors.
        computer->Traverse(this);
    }

    void UpdateVisitor::VisitHardware(IHardware^ hardware)
    {
        // This updates the specific piece of hardware (e.g., CPU, GPU).
        hardware->Update();
        // This recursively visits all sub-hardware components.
        for each (IHardware^ subHardware in hardware->SubHardware)
        {
            subHardware->Accept(this);
        }
    }

    void UpdateVisitor::VisitSensor(ISensor^ sensor)
    {
        // This method is required by the interface but is intentionally empty.
        // The actual sensor values are updated when VisitHardware calls hardware->Update().
    }

    void UpdateVisitor::VisitParameter(IParameter^ parameter)
    {
        // This method is also required but not needed for this implementation.
    }
}