#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <FL/Fl.h>
#include <FL/fl_draw.h>
#include <Fl/math.h>
#include <Fl/gl.h>
#include <GL/glu.h>
#include <iostream>
#include <iomanip>
#include <random>
#include <climits>
#include "GluFunction.h"

float FuncMatrix::modelView[16] = {};
float FuncMatrix::projection[16] = {};
random_device rd;
default_random_engine generator(rd());
uniform_real_distribution<float> unif(0.0f, 1.0f);

ostream& operator<<(ostream& ostrm, const Direction& dir)
{
	switch (dir)
	{
	case Direction::INCLUDING:
		ostrm << "in";
		break;
	case Direction::RIGHT:
		ostrm << "right";
		break;
	case Direction::LEFT:
		ostrm << "left";
		break;
	}

	return ostrm;
}

ostream& operator<<(ostream& ostrm, const Vector& vec)
{
	ostrm << vec.X << " " << vec.Y << " " << vec.Z << " " << vec.W;
	return ostrm;
}

void MyGlu::Normalization(float vec[3])
{
	float len = sqrt(pow(vec[0], 2) + pow(vec[1], 2) + pow(vec[3], 2));
	vec[0] /= len;
	vec[1] /= len;
	vec[2] /= len;
}

void MyGlu::Normalization(Vector& target)
{
	target = target / VectorLength(target);
}

float MyGlu::AngleNormalization(float dir)
{
	int t = dir / 360;
	if (t > 0)
		return dir - t * 360.0f;
	else if (t < 0)
		return dir + (-t + 1) * 360.0f;
	
	return dir;
}

float MyGlu::VectorLength(Vector vec)
{
	float length = sqrt(vec.X * vec.X + vec.Y * vec.Y + vec.Z * vec.Z);

	return length;
}

Vector MyGlu::CrossProduct(Vector vec1, Vector vec2)
{
	Vector result;
	result.X = vec1.Y * vec2.Z - vec1.Z * vec2.Y;
	result.Y = vec1.Z * vec2.X - vec1.X * vec2.Z;
	result.Z = vec1.X * vec2.Y - vec1.Y * vec2.X;

	return result;
}

void MyGlu::MatrixProduct4x4(float ans[16], float* mat1, float* mat2)
{
	float sum = 0;
	int base;
	
	for (int x = 0; x < 4; x++)
	{
		base = x * 4;
		for (int y = 0; y < 4; y++)
		{
			sum = 0;
			for (int z = 0; z < 4; z++)
			{
				sum += mat1[base + z] * mat2[z * 4 + y];
			}
			ans[x * 4 + y] = sum;
		}
	}
}

void MyGlu::MatrixProduct4x1(float ans[4], float* mat1, float* mat2)
{
	float sum = 0;
	int base;

	for (int x = 0; x < 4; x++)
	{
		sum = 0;
		for (int y = 0; y < 4; y++)
		{
			sum += mat1[x + y * 4] * mat2[y];
		}
		ans[x] = sum;
	}
}

void MyGlu::Frustum(float left, float right, float bottom, float top, float znear, float zfar)
{
	float P[16] = { 0 };
	P[0] = (2 * znear) / (right - left);
	P[5] = (2 * znear) / (top - bottom);
	P[8] = (right + left) / (right - left);
	P[9] = (top + bottom) / (top - bottom);
	P[10] = -(zfar + znear) / (zfar - znear);
	P[11] = -1;
	P[14] = -(2 * zfar * znear) / (zfar - znear);
	for (int i = 0; i < 16; i++)
		FuncMatrix::projection[i] = P[i];
	//glLoadMatrixf(P);
}

void MyGlu::LookAt(Position eye, Position center, Vector up)
{
	float rotation[16] = {};
	float translation[16] = {};
	float lookAtMatrix[16] = {};


	Vector w(center - eye);

	Vector u = CrossProduct(w, up);
	Normalization(u);

	Vector v = CrossProduct(u, w);
	Normalization(v);

	rotation[0] = u.X;
	rotation[4] = u.Y;
	rotation[8] = u.Z;

	rotation[1] = v.X;
	rotation[5] = v.Y;
	rotation[9] = v.Z;

	rotation[2] = -w.X;
	rotation[6] = -w.Y;
	rotation[10] = -w.Z;

	rotation[15] = 1;

	translation[0] = translation[5] = translation[10] = 1.0f;
	translation[12] = -eye.X;
	translation[13] = -eye.Y;
	translation[14] = -eye.Z;
	translation[15] = 1.0f;

	MatrixProduct4x4(lookAtMatrix, translation, rotation);
	for (int i = 0; i < 16; i++)
		FuncMatrix::modelView[i] = lookAtMatrix[i];
	//glLoadMatrixf(lookAtMatrix);
}

void MyGlu::Perspective(float fovy, float aspect, float znear, float zfar)
{
	float yLength = znear * tanf(fovy / 360.0f * PI);
	float xLength = yLength * aspect;
	Frustum(-xLength, xLength, -yLength, yLength, znear, zfar);
}

Direction MyGlu::PointDirection(LineSeg line, float point[2])
{
	float vec1[2] = { line.start[0] - line.end[0], line.start[1] - line.end[1] };
	float vec2[2] = { point[0] - line.start[0], point[1] - line.start[1] };
	float sum = vec1[0] * vec2[1] - vec1[1] * vec2[0];
	if (sum > 0)
		return Direction::RIGHT;
	else if (sum < 0)
		return Direction::LEFT;
	else
		return Direction::INCLUDING;
}

void MyGlu::Vertex3(Vector vec)
{
	float mat[4] = { vec.X, vec.Y, vec.Z, vec.W};
	float ans[4] = { 0 };

	MatrixProduct4x1(ans, FuncMatrix::modelView, mat);	
	for (int i = 0; i < 4; i++)
		mat[i] = ans[i];	
	MatrixProduct4x1(ans, FuncMatrix::projection, mat);

	Vector vec2(ans);
	if (vec2.W <= 0)
		return;
	vec2 = vec2 / vec2.W;
	glVertex2f(vec2.X, vec2.Y);
}

void MyGlu::ShowMat(float* mat, int xSize,int ySize)
{
	int base = 0;
	for (int x = 0; x < xSize; x++)
	{
		base = x * 4;
		for (int y = 0; y < ySize; y++)
		{
			cout << setw(12) << mat[base + y];
		}
		cout << "\n";
	}
}

void MyGlu::GenerateColor(float* color)
{
	for (int i = 0; i < 3; i++)
	{
		color[i] = unif(generator);
	}
}