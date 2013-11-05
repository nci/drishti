#pragma once

class Point
{
public:
	Point(int u, int v)
	{
		this->x = u;
		this->y = v;
	}

	Point() {};

	int x;
	int y;
};