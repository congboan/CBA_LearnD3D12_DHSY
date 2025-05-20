#pragma once
#include "DirectXMath.h"

class Camera
{
public:
    DirectX::XMMATRIX m_viewMatrix;

    DirectX::XMVECTOR m_position, m_focusPoint;

    void Update(float inPositiontX, float inPositiontY, float inPositiontZ, float inFocusPointX, float inFocusPointY,
                float inFocusPointZ, float inUpX, float inUpY, float inUpZ);
};
