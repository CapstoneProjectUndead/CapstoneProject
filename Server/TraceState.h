#pragma once
// ServerÂÊ TraceState
#include "State.h"

class CTraceState :
    public CState
{
public:
    CTraceState();
    ~CTraceState();

    virtual void Update(float elapsedTime) override;
    virtual void Enter() override;
    virtual void Exit() override;
};

