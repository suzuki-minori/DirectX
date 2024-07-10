#pragma once
struct Matrix4x4 {
	float m[4][4];
};


//struct Vector4 {
//	float x;
//	float y;
//	float z;
//	float w;
//};

//

//
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);
//
Matrix4x4 Substract(const Matrix4x4& m1, Matrix4x4& m2);
//
Matrix4x4 Multiply(const Matrix4x4& m1, Matrix4x4& m2);
//
Matrix4x4 Inverse(const Matrix4x4& m);
//
Matrix4x4 Transpose(const Matrix4x4& m);
//
Matrix4x4 MakeIdentity4x4();
//
Matrix4x4 MakeAffineMatrix();