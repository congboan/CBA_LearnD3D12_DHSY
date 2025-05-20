#include "Camera.h"

void Camera::Update(float inPositionX, float inPositionY, float inPositionZ, float inFocusPointX, float inFocusPointY,
                    float inFocusPointZ, float inUpX, float inUpY, float inUpZ)
{
    m_position = DirectX::XMVectorSet(inPositionX, inPositionY, inPositionZ, 1.0f);
    m_focusPoint = DirectX::XMVectorSet(inFocusPointX, inFocusPointY, inFocusPointZ, 1.0f);
    m_viewMatrix = DirectX::XMMatrixLookAtLH(m_position, m_focusPoint, DirectX::XMVectorSet(inUpX, inUpY, inUpZ, 0.0f));

}
