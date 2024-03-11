#pragma once
#include <iostream>
#include <iomanip>
#include <GL/GL.h>
#include "LineSeg.h"
#define PI 3.14159265358

using namespace std;

struct FuncMatrix
{
	static float modelView[16];
	static float projection[16];
};

struct Vector
{
	float X;
	float Y;
	float Z;
	float W;
	Vector(float x = 0, float y = 0, float z = 0, float w = 0) :X(x), Y(y), Z(z), W(w){}
	Vector(float val[4])
	{
		X = val[0];
		Y = val[1];
		Z = val[2];
		W = val[3];
	}
	Vector operator/(float a)
	{
		return Vector(X / a, Y / a, Z / a, W / a);
	}
};

struct Position
{
	float X;
	float Y;
	float Z;
	Position(float x = 0, float y = 0, float z = 0) :X(x), Y(y), Z(z) {}
	Position(float *pos) : X(pos[0]), Y(pos[1]), Z(pos[2]) {}
	Vector operator-(Position a)
	{
		return Vector(X - a.X, Y - a.Y, Z - a.Z);
	}
};

enum class Direction
{
	RIGHT, LEFT, INCLUDING
};

ostream& operator<<(ostream& ostrm, const Vector& vec);
ostream& operator<<(ostream& ostrm, const Direction& dir);

namespace MyGlu
{
	void       Normalization(float vec[3]);
		       
	void       Normalization(Vector& target);
		       
	float      AngleNormalization(float dir);
		       
	void       MatrixProduct4x4(float ans[16], float* mat1, float* mat2);
		       
	void       MatrixProduct4x1(float ans[4], float* mat1, float* mat2);
		       
	Vector     CrossProduct(Vector vec1, Vector vec2);
		       
	float      VectorLength(Vector vec);
		       
	void       LookAt(Position eye, Position center, Vector up);
		       
	void       Frustum(float left, float right, float bottom, float top, float znear, float zfar);
		       
	void       Perspective(float fovy, float aspect, float znear, float zfar);
			   
	Direction  PointDirection(LineSeg line, float point[2]);
			   
	void       Vertex3(Vector vec);
			   
	void       ShowMat(float* mat, int xSize, int ySize);

	void	   GenerateColor(float *color);
}
