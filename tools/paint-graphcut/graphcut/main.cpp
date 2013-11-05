#include "graph.h"
#include "point.h"

#include <iostream>
#include <fstream>

#include <math.h>
#include <vector>

#include "CImg.h"

using namespace cimg_library;
using namespace std;

//#define TEST

#ifdef TEST
#define ZOOM 5
#else
#define ZOOM 1
#endif

#define INF 100000000
#define diff(img, x1, y1, x2, y2) pow(img(x1,y1) - img(x2,y2),2)

#ifdef TEST
  #define VOIS 2
#else
  #define VOIS 10
#endif


int BOUNDARY_BALLOONING = 0;
int UNIFORM_BALLOONING = 1;
int FORCE_BALLOONING = 2;


typedef Graph<double,double,double> GraphType;

/**
* Affiche de l'image à l'écran
**/
//template <class T> 
//void display_image(CImg<T> &image, int delay = 0)
//{
//	CImgDisplay display(image, "Image");
//	if (delay != 0)
//		display.wait(delay);
//	else
//		display.wait();
//}

inline int round(double x)
{
	int fl = (int) floor(x);
	if (x-fl > 0.5)
		return fl+1;
	else
		return fl;
}


void compute_sigmas(CImg<double> &image, CImg<double> &sigmas)
{
	int w=image.width;
	int h=image.height;

	for (int i=0; i<w; ++i)
	{
		for (int j=0; j<h; ++j)
		{
			double sum=0;
			int xmax = min(w-1, i+VOIS);
			int ymax = min(h-1, j+VOIS);
			int xmin = max(0,i-VOIS);
			int ymin = max(0,j-VOIS);

			for (int x=xmin; x<xmax; ++x)
			{
				for (int y=ymin; y<ymax; ++y)
				{
					double tmp = image(x,y);

					sum += abs(tmp-image(x+1,y));
					sum += abs(tmp-image(x,y+1));
				}
			}

			sigmas(i,j) = sum / (2*(xmax-xmin)*(ymax-ymin));

		}
	}

}



/**
* Find object points and background points in the mask image
**/
void find_points(CImg<double> &mask,
				 std::vector<Point> &object_points,
				 std::vector<Point> &background_points,
				 Point &star_point)
{
	for (unsigned int i=0; i<mask.width; ++i)
	{
		for (unsigned int j=0; j<mask.height; ++j)
		{
			double val = mask(i,j);
			if (val == 1)
				object_points.push_back(Point(i,j));
			else if (val == 0)
				background_points.push_back(Point(i,j));
			else if (val == 2)
				star_point = Point(i,j);
		}
	}
}


/**
* Set edges correponding to a line starting from the border of the image to the star point
**/
inline void draw_line(CImg<bool> &edges_status_out, GraphType &G,
					  int &x, int &y, int &X, int &Y,
					  int &nx, double &beta, bool &auto_background, bool &star_shape_prior,
					  CImg<double> &edges)
{
	if (auto_background)
		G.add_tweights(y*nx+x, INF, 0);

	if (!star_shape_prior)
		return;

	int dx = (x<X) ? 1 : (-1);
	int dy = (y<Y) ? 1 : (-1);

	//Current point: (i,j)
	int i,j;

	if (abs(Y-y) > abs(X-x))
	{
		double slopey = 1.*(X-x)/(Y-y);
		//Parcours en y
		//cout << "Parcours en y" << endl;
		for (int j=y; j != Y; j+=dy)
		{
			i = round((j-y)*slopey + x);

			//Next point: ((j+dy-y)*slopey + x, j+dy)
			int next_i = round((j+dy-y)*slopey + x);
			int next_j = j+dy;

			dx = next_i - i;

			G.add_edge(j*nx + i, next_j*nx + next_i, beta, INF);
#ifdef TEST
			//edges.draw_arrow(ZOOM*i, ZOOM*j, ZOOM*next_i, ZOOM*next_j, &col, 20, 10);
			double col = 0;
			edges.draw_line(ZOOM*i, ZOOM*j, ZOOM*next_i, ZOOM*next_j, &col);
#endif

			if (   (!edges_status_out(next_i, next_j)) ||
				   ((next_i == X) && (next_j == Y))   )
			{
				edges_status_out(next_i, next_j) = true;
			}
			else
				return;
		}
	}
	else
	{
		//Parcours en x
		//cout << "Parcours en x" << endl;
		double slopex = 1.*(Y-y)/(X-x);

		for (int i=x; i != X; i+=dx)
		{
			j = round(slopex * (i-x) + y);

			//Next point: (i+dx, slope * (i+dx-x) + y)
			int next_i = i+dx;
			int next_j = round(slopex * (i+dx-x) + y);

			dy = next_j - j;

			G.add_edge(j*nx + i, next_j*nx + next_i, beta, INF);
#ifdef TEST
			double col = 0;
			edges.draw_line(ZOOM*i, ZOOM*j, ZOOM*next_i, ZOOM*next_j, &col);
			//edges.draw_arrow(ZOOM*i, ZOOM*j, ZOOM*next_i, ZOOM*next_j, &col, 20, 10);
#endif
			if (   (!edges_status_out(next_i, next_j)) ||
				   ((next_i == X) && (next_j == Y))   )
			{
				edges_status_out(next_i, next_j) = true;
			}
			else

				return;

		}
	}
	
}



inline void draw_edges_image_data(GraphType &G, CImg<double> &image,
								  int &x1, int &y1, int x2, int y2, int &nx, int &ny,
								  double &lambda, CImg<double> &sigmas, double &sigma_hard, double dist,
								  CImg<double> &edges)
{
	if (x2 >= 0 && x2<nx && y2>=0 && y2 < ny)
	{
		double sigma;
		if (sigma_hard != 0)
			sigma = sigma_hard;
		else
			sigma = sigmas(x1, y1);

		double weight = lambda * exp(-diff(image, x1, y1, x2, y2)/(2*sigma*sigma)) / dist;
		if (sigma == 0)
			weight = 0;

		G.add_edge(y1*nx+x1, y2*nx+x2, weight, weight);	//Voisin du haut
		double col = 0;
#ifdef TEST
		edges.draw_line(ZOOM*x1, ZOOM*y1, ZOOM*x2, ZOOM*y2, &col);
#endif
	}
}



int main( int argc, char *argv[]  )
{
	CImg<double> image;
	CImg<double> mask;

	//Algo parameters
	double lambda;
	double sigma = 0;
	double beta = 0;
	double force_type = 0;
	double force;

	bool auto_background;
	bool star_shape_prior;
	bool compute_beta = false;

#ifndef TEST
	lambda = atof(argv[1]);
	sigma  = atof(argv[2]);
	
	auto_background  = (atof(argv[3]) == 1);
	star_shape_prior = (atof(argv[4]) == 1);
	
	force_type = atof(argv[5]);
	force = atof(argv[6]);

	if (force_type == BOUNDARY_BALLOONING)
	{
		beta = force;

		if (force == -234.12)
			compute_beta = true;
	}

	cout << "Force type=" << force_type << endl;
	cout << "Lambda=" << lambda << endl;
	cout << "Sigma=" << sigma << endl;
	if (!compute_beta)
		cout << "Beta=" << beta << endl;
	else
		cout << "Optimal beta search activated" << endl;
	cout << "Auto-Background=" << auto_background << endl;
	cout << "Star Shape=" << star_shape_prior << endl;
#else
	cout << "TEST " << endl << endl;
	//Parameters
	lambda = 10;
	beta = -6;
	auto_background = true;
	star_shape_prior = true;

#endif

	//Loads images
	image.load("image.bmp");
	//image = image.RGBtoLab();
	mask.load("mask.bmp");
	mask = mask.get_channel(0);

	unsigned int w = image.width;
	unsigned int h = image.height;

	CImg<double> sigmas(w,h);
	if (sigma == 0)
		compute_sigmas(image, sigmas);

#ifdef TEST
	//display_image(sigmas);
#endif
	sigmas.save("sigmas.bmp");

	//Loads object.background seeds
	std::vector<Point> object_points;
	std::vector<Point> background_points;
	Point star_point(-1,-1);
	find_points(mask, object_points, background_points, star_point);
	cout << "Star point is: " << star_point.x << " " << star_point.y << endl;
	cout << object_points.size() << " object points." << endl;
	cout << background_points.size() << " background points." << endl;

	if (star_point.x == -1)
	{
		cout << "No star point provided..." << endl;
		system("pause");
		return 1;
	}

	int X = star_point.x;
	int Y = star_point.y;

#ifdef TEST
	//Display points coordinates
	cout << "Object points:" << endl;
	for (unsigned int i=0; i<object_points.size(); ++i)
		cout << object_points[i].x << " " << object_points[i].y << endl;
	cout << endl;

	cout << "Background points:" << endl;
	for (unsigned int i=0; i<background_points.size(); ++i)
		cout << background_points[i].x << " " << background_points[i].y << endl;
	cout << endl;

#endif




	CImg<double> edges(w*ZOOM-(ZOOM-1), h*ZOOM-(ZOOM-1), 1, 1, 255);




	//Finding optimal beta
	double beta_min = -30;
	double beta_max = 0;
	int iters_max = 10;

	double beta_sup = beta_max;
	double beta_inf = beta_min;
	if (compute_beta)
		beta = (beta_sup + beta_inf)/2;

	int nb_iters = 0;
	int nx=w, ny=h;	// Sans les bords
	GraphType G(nx*ny,32*nx*ny);

	while (true)
	{
		cout << "Iteration " << nb_iters << endl;
		cout << "Beta=" << beta << endl;

		// Graphe
		cout << "Setting graph...";

		G.reset();
		G.add_node(nx*ny);

		//Data term
		for (int i=0; i<nx; i++)
		{
			for (int j=0; j<ny; j++)
			{
				G.add_tweights(j*nx+i, 0, 0);

				draw_edges_image_data(G, image, i, j, i, j+1, nx, ny, lambda, sigmas, sigma, 1, edges);	//Down
				draw_edges_image_data(G, image, i, j, i+1, j, nx, ny, lambda, sigmas, sigma, 1, edges);	//Right
				draw_edges_image_data(G, image, i, j, i+1, j-1, nx, ny, lambda, sigmas, sigma, sqrt(2.), edges);	//Top right
				draw_edges_image_data(G, image, i, j, i+1, j+1, nx, ny, lambda, sigmas, sigma, sqrt(2.), edges);	//Down right

			}
		}

#ifdef TEST
		edges.fill(255);
#endif

		//Seed object points
		for (unsigned int i=0; i<object_points.size(); ++i)
		{
			int x = object_points[i].x;
			int y = object_points[i].y;
			G.add_tweights(y*nx+x, 0, INF);
		}

		//Star point
		G.add_tweights(Y*nx+X, 0, INF);

		//Background object points
		for (unsigned int i=0; i<background_points.size(); ++i)
		{
			int x = background_points[i].x;
			int y = background_points[i].y;
			G.add_tweights(y*nx+x, INF, 0);
		}



		////Star shape edges for each line from the center to any pixel
		//CImg<bool> edges_status_out(w, h, 9, 1, false);
		//for (int x=0; x<w; ++x)
		//{
		//	for (int y=0; y<h; ++y)
		//	{
		//		if ((abs(x-X) + abs(y-Y)) > 2)
		//			draw_line(edges_status_out, G, x, y, X, Y, nx, beta, auto_background, star_shape_prior, edges);
		//	}
		//}


        //Star shape edges for each line from C to the border
        CImg<bool> edges_status_out(w, h, 1, 1, false);
		cimg_for_borderXY(image, x, y, 1)
			draw_line(edges_status_out, G, x, y, X, Y, nx, beta, auto_background, star_shape_prior, edges);

		cout << "Done." << endl;

		if (force_type == UNIFORM_BALLOONING)
		{
			//Ballooning force
			double k = 1;
			for (int x=0; x<w; ++x)
			{
				for (int y=0; y<h; ++y)
				{
					if (!(x==X && y==Y))
					{
						G.add_tweights(y*nx+x, -force, 0);
					}
				}
			}
		}

		if (force_type == FORCE_BALLOONING)
		{
			//Ballooning force
			for (int x=0; x<w; ++x)
			{
				for (int y=0; y<h; ++y)
				{
					if (!(x==X && y==Y))
					{
						double tweight = - force * 1. / sqrt((double) ((x-X)*(x-X) + (y-Y)*(y-Y)) );
						G.add_tweights(y*nx+x, tweight, 0);

						//double force = - k * 1. / sqrt((double) ((x-X)*(x-X) + (y-Y)*(y-Y)) );
						//G.add_tweights(y*nx+x, force, 0);
					}
				}
			}
		}



#ifdef TEST
		//display_image(edges.get_resize(-400, -400), 1000);
		edges.save("star_prior_term.bmp");
#endif

		// Coupe
		cout << "Computing Cut..." << endl;
		double f = G.maxflow();
		cout << f << endl;

		if (!compute_beta)
			break;


		// Segmentation result
		int nb_objects=0;
		for (int j=0;j<ny;j++) {
			for (int i=0;i<nx;i++) {
				if (G.what_segment(j*nx+i)==GraphType::SINK)
					nb_objects++;
			}
		}
		cout << "Object contains " << nb_objects << " points" << endl << endl;

		//New beta
		if (nb_objects < 1000)
		{
			//On baisse le beta
			beta_inf = beta_inf;
			beta_sup = beta;
			beta = (beta_inf + beta)/2;

			if (beta - beta_min < 0.05)
			{
				cout << "Diminution du beta..." << endl;
				beta_inf -= 50;
				beta_min = beta_inf;
				beta_sup = beta;
				beta = (beta_inf + beta)/2;
				nb_iters = 0;
			}
		}
		else
		{
			if (nb_iters >= iters_max)
				break;

			//On augmente le beta
			beta_inf = beta;
			beta_sup = beta_sup;
			beta = (beta_sup + beta)/2;

			if (beta_max - beta < 0.05)
			{
				cout << "augmentation du beta..." << endl;
				beta_sup += 50;
				beta_max = beta_sup;
				beta_inf = beta;
				beta = (beta_sup + beta)/2;
				nb_iters = 0;
			}
		}

		nb_iters++;
	}


	// Dessin de la zone objet
	for (int j=0;j<ny;j++) {
		for (int i=0;i<nx;i++) {
			if (G.what_segment(j*nx+i)==GraphType::SINK)
			{
				image(i,j,0) = image(i,j,0)*0.5 + 255 * 0.5;
				image(i,j,1) = image(i,j,1)*0.5;
				image(i,j,2) = image(i,j,2)*0.5;
				//image(i,j,0) = 0;
				//image(i,j,1) = 0;
				//image(i,j,1) = image(i,j,1)*0.5 + 255 * 0.5;
			}
		}
	}

	//Dessin object points
	for (unsigned int in=0; in<object_points.size(); ++in)
	{
		for (int i=-2; i<=2; ++i)
		{
			for (int j=-2; j<=2; ++j)
			{
				image(object_points[in].x+i, object_points[in].y+j, 0) = 255;
				image(object_points[in].x+i, object_points[in].y+j, 1) = 255;
				image(object_points[in].x+i, object_points[in].y+j, 2) = 0;
			}
		}
	}

	//Star point
	for (int i=-2; i<=2; ++i)
	{
		for (int j=-2; j<=2; ++j)
		{
			image(X+i, Y+j, 0) = 0;
			image(X+i, Y+j, 1) = 0;
			image(X+i, Y+j, 2) = 255;
		}
	}

	//Dessin background points
	for (unsigned int in=0; in<background_points.size(); ++in)
	{
		for (int i=-2; i<=2; ++i)
		{
			for (int j=-2; j<=2; ++j)
			{
				image(background_points[in].x+i, background_points[in].y+j, 0) = 0;
				image(background_points[in].x+i, background_points[in].y+j, 1) = 255;
				image(background_points[in].x+i, background_points[in].y+j, 2) = 255;
			}
		}
	}



#ifdef TEST
	//display_image(image);
	system("pause");
#endif

	image.save("results.bmp");

	return 0;
}
