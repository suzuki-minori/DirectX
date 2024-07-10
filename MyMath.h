#pragma once
#include"Vector3.h"
#include"Matrix4x4.h"


class MyMath
{
public:

	static const int kColumnWidth = 60;

	//加算
	static Vector3 Add(const Vector3& v1, const Vector3& v2);
	//減算
	static Vector3 Subtract(const Vector3& v1, const Vector3& v2);
	//スカラー倍
	static Vector3 Multiply(float scalar, const Vector3& v);
	//内積
	static float Dot(const Vector3& v1, const Vector3& v2);
	//長さ(ノルム)
	static float Length(const Vector3& v);
	//正規化
	static Vector3 Normalize(const Vector3& v);
	//座標変換
	static Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);

};

