#pragma once
#include"Vector4.h"
#include"Vector3.h"
	struct DirectionalLight {
		Vector4 color;
		Vector3 direction;//向き
		float intensity;//輝度
	};


