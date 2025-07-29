#include "pch.h"
#include "CameraComponent.h"

void CameraComponent::FinalUpdate(Matrix mat)
{
	_matView = mat;



	if (_type == PROJECTION_TYPE::PERSPECTIVE)
		_matProjection = ::XMMatrixPerspectiveFovLH(_fov, _width / _height, _near, _far);
	else
		_matProjection = ::XMMatrixOrthographicLH(_width * _scale, _height * _scale, _near, _far);


	_frustum.FinalUpdate(_matView, _matProjection);
}
