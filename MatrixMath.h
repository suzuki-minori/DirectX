#pragma once
#include"Matrix4x4.h"
#include"Vector3.h"
#include<cmath>



class MatrixMath
{
public:

	static Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);
	static Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);
	static Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
	static Matrix4x4 Inverse(const Matrix4x4& m);
	static Matrix4x4 Transpose(const Matrix4x4 m);
	static Matrix4x4 MakeIdentity4x4();

	//平行移動行列
	static Matrix4x4 MakeTranslateMatrix(const Vector3& translate);
	//拡縮行列
	static Matrix4x4 MakeScaleMatrix(const Vector3& scale);
	//X軸回転行列
	static Matrix4x4 MakeRotateXMatrix(float radian);
	//
	static Matrix4x4 MakeRotateYMatrix(float radian);
	//
	static Matrix4x4 MakeRotateZMatrix(float radian);

	//
	static Matrix4x4 Multiply(Matrix4x4& m1, Matrix4x4& m2);

	//
	static Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);
	//
	static Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

};

