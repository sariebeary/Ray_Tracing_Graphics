// CS3241Lab5.cpp 
#include <cmath>
#include <iostream>
#include "GL\glut.h"
#include "vector3D.h"
#include <chrono>

using namespace std;

#define WINWIDTH 600
#define WINHEIGHT 400
#define NUM_OBJECTS 4
#define MAX_RT_LEVEL 50
#define NUM_SCENE 2

float* pixelBuffer = new float[WINWIDTH * WINHEIGHT * 3];

class Ray { // a ray that start with "start" and going in the direction "dir"
public:
	Vector3 start, dir;
};

class RtObject {

public:
	virtual double intersectWithRay(Ray, Vector3& pos, Vector3& normal) = 0; // return a -ve if there is no intersection. Otherwise, return the smallest postive value of t

	// Materials Properties
	double ambiantReflection[3] ;
	double diffusetReflection[3] ;
	double specularReflection[3] ;
	double speN = 300;


};

class Sphere : public RtObject {

	Vector3 center_;
	double r_;
public:
	Sphere(Vector3 c, double r) { center_ = c; r_ = r; };
	Sphere() {};
	void set(Vector3 c, double r) { center_ = c; r_ = r; };
	double intersectWithRay(Ray, Vector3& pos, Vector3& normal);
};

RtObject **objList; // The list of all objects in the scene


// Global Variables
// Camera Settings
Vector3 cameraPos(0,0,-500);

// assume the the following two vectors are normalised
Vector3 lookAtDir(0,0,1);
Vector3 upVector(0,1,0);
Vector3 leftVector(1, 0, 0);
float focalLen = 500;

// Light Settings

Vector3 lightPos(900,1000,-1500);
double ambiantLight[3] = { 0.4,0.4,0.4 };
double diffusetLight[3] = { 0.7,0.7, 0.7 };
double specularLight[3] = { 0.5,0.5, 0.5 };


double bgColor[3] = { 0.1,0.1,0.4 };

int sceneNo = 0;


double Sphere::intersectWithRay(Ray r, Vector3& intersection, Vector3& normal)
// return a -ve if there is no intersection. Otherwise, return the smallest postive value of t
{

	// Step 1 Light Ray Equation 
	Vector3 d = r.dir; // direction vector of the ray 
	Vector3 p = r.start; // origin of the ray
	Vector3 c = center_; // center of sphere 

	double alpha = dot_prod(d, d); 
	double beta = dot_prod(d * 2.0, (p - c));
	double gamma = ((p - c).lengthsqr()) - pow(r_, 2); 
	double det = pow(beta, 2) - (4 * alpha * gamma); // determinant
	double t; 

	// 2 roots 
	if(det > 0) {
		double t1 = (-beta + sqrt(det)) / (2 * alpha);
		double t2 = (-beta - sqrt(det)) / (2 * alpha);
		// smaller positive root gives the nearer intersection 
		if (t1 > 0 && t2 > 0) {
			if (t1 > t2) {
				t = t2;
			}
			else {
				t = t1; 
			}
		}
		else if (t1 > 0) {
			t = t1; 
		}
		else if (t2 > 0) {
			t = t2; 
		}
		else {
		// negative root behind ray origin (consider no interection) 
			return -1; 
		}

		// intersection
		intersection = p + (d * t);

		// normal
		normal = (intersection - c);
		normal.normalize();
		return t; 
	}
	
	// imaginary roots or 1 root(ray tangential to sphere so consider no intersection) 
	return -1;
}


void rayTrace(Ray ray, double& r, double& g, double& b, int fromObj = -1 ,int level = 0)
{
	// Step 4
	if (level == MAX_RT_LEVEL) {
		return;
	}

	int goBackGround = 1, i = 0, j = 0;

	Vector3 intersection, normal;
	Vector3 lightV;
	Vector3 viewV;
	Vector3 lightReflectionV;
	Vector3 rayReflectionV;

	Ray newRay;
	double mint = DBL_MAX, t;
	bool shadow = false; 


	for (i = 0; i < NUM_OBJECTS; i++)
	{
		if ((t = objList[i]->intersectWithRay(ray, intersection, normal)) > 0)
		{
			//r = g = b = 1.0; 			
			// a ray will not hit objects where it rebounds froms 
			if ((t < mint) && (fromObj != i)) {
				mint = t;
				// Step 2 
				r = ambiantLight[0] * objList[i]->ambiantReflection[0];
				g = ambiantLight[1] * objList[i]->ambiantReflection[1];
				b = ambiantLight[2] * objList[i]->ambiantReflection[2];

				// Step 3
				goBackGround = 0;
				// check shadow ray from each surface point to each light source 
				Ray shadowRay;
				shadowRay.start = intersection;
				shadowRay.dir = lightV; 

				for (j = 0; j < NUM_OBJECTS; j++)
				{
					// if hit nothing but light source, PIE 
					if ((i != j) && (t = objList[j]->intersectWithRay(shadowRay, intersection, normal)) > 0)
					{// else ambiant 
						shadow = true; 
					}
				}

				if (!shadow) {
					// diffuse reflection
					lightV = (lightPos - intersection); // direction unit vector from surface point to light source  
					lightV.normalize();
					r += diffusetLight[0] * objList[i]->diffusetReflection[0] * dot_prod(normal, lightV);
					g += diffusetLight[1] * objList[i]->diffusetReflection[1] * dot_prod(normal, lightV);
					b += diffusetLight[2] * objList[i]->diffusetReflection[2] * dot_prod(normal, lightV);

					// specular reflection 
					lightReflectionV = normal * 2 * dot_prod(normal, lightV) - lightV; // reflection vector 2(N dot L) N - L
					viewV = cameraPos - intersection; // direction unit vector from the surface point to viewer  
					viewV.normalize();
					r += specularLight[0] * objList[i]->specularReflection[0] * pow(dot_prod(lightReflectionV, viewV), objList[i]->speN);
					g += specularLight[1] * objList[i]->specularReflection[1] * pow(dot_prod(lightReflectionV, viewV), objList[i]->speN);
					b += specularLight[2] * objList[i]->specularReflection[2] * pow(dot_prod(lightReflectionV, viewV), objList[i]->speN);
				
				}

				rayReflectionV = normal * 2 * dot_prod(normal, -ray.dir) + ray.dir; // 2 (N dot (-i)) N + i
				rayReflectionV.normalize();
				newRay.start = intersection;
				newRay.dir = rayReflectionV;

				double reflectedR, reflectedG, reflectedB = 1.0;
				rayTrace(newRay, reflectedR, reflectedG, reflectedB, i, level + 1);
				r += reflectedR * objList[i]->specularReflection[0];
				g += reflectedG * objList[i]->specularReflection[1];
				b += reflectedB * objList[i]->specularReflection[2];
			}
		}
	}

	if (goBackGround)
	{
		r = bgColor[0];
		g = bgColor[1];
		b = bgColor[2];
	}

}


void drawInPixelBuffer(int x, int y, double r, double g, double b)
{
	pixelBuffer[(y*WINWIDTH + x) * 3] = (float)r;
	pixelBuffer[(y*WINWIDTH + x) * 3 + 1] = (float)g;
	pixelBuffer[(y*WINWIDTH + x) * 3 + 2] = (float)b;
}

void renderScene()
{
	int x, y;
	Ray ray;
	double r, g, b;

	cout << "Rendering Scene " << sceneNo << " with resolution " << WINWIDTH << "x" << WINHEIGHT << "........... ";
	__int64 time1 = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count(); // marking the starting time

	ray.start = cameraPos;

	Vector3 vpCenter = cameraPos + lookAtDir * focalLen;  // viewplane center
	Vector3 startingPt = vpCenter + leftVector * (-WINWIDTH / 2.0) + upVector * (-WINHEIGHT / 2.0);
	Vector3 currPt;

	for(x=0;x<WINWIDTH;x++)
		for (y = 0; y < WINHEIGHT; y++)
		{
			currPt = startingPt + leftVector*x + upVector*y;
			ray.dir = currPt-cameraPos;
			ray.dir.normalize();
			rayTrace(ray, r, g, b);
			drawInPixelBuffer(x, y, r, g, b);
		}

	__int64 time2 = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count(); // marking the ending time

	cout << "Done! \nRendering time = " << time2 - time1 << "ms" << endl << endl;
}


void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |GL_DOUBLEBUFFER);
	glDrawPixels(WINWIDTH, WINHEIGHT, GL_RGB, GL_FLOAT, pixelBuffer);
	glutSwapBuffers();
	glFlush ();
} 

void reshape (int w, int h)
{
	glViewport (0, 0, (GLsizei) w, (GLsizei) h);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();

	glOrtho(-10, 10, -10, 10, -10, 10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


void setScene(int i = 0)
{
	if (i > NUM_SCENE)
	{
		cout << "Warning: Invalid Scene Number" << endl;
		return;
	}

	if (i == 0)
	{

		((Sphere*)objList[0])->set(Vector3(-130, 80, 120), 100);
		((Sphere*)objList[1])->set(Vector3(130, -80, -80), 100);
		((Sphere*)objList[2])->set(Vector3(-130, -80, -80), 100);
		((Sphere*)objList[3])->set(Vector3(130, 80, 120), 100);

		objList[0]->ambiantReflection[0] = 0.1;
		objList[0]->ambiantReflection[1] = 0.4;
		objList[0]->ambiantReflection[2] = 0.4;
		objList[0]->diffusetReflection[0] = 0;
		objList[0]->diffusetReflection[1] = 1;
		objList[0]->diffusetReflection[2] = 1;
		objList[0]->specularReflection[0] = 0.2;
		objList[0]->specularReflection[1] = 0.4;
		objList[0]->specularReflection[2] = 0.4;
		objList[0]->speN = 300;

		objList[1]->ambiantReflection[0] = 0.6;
		objList[1]->ambiantReflection[1] = 0.6;
		objList[1]->ambiantReflection[2] = 0.2;
		objList[1]->diffusetReflection[0] = 1;
		objList[1]->diffusetReflection[1] = 1;
		objList[1]->diffusetReflection[2] = 0;
		objList[1]->specularReflection[0] = 0.0;
		objList[1]->specularReflection[1] = 0.0;
		objList[1]->specularReflection[2] = 0.0;
		objList[1]->speN = 50;

		objList[2]->ambiantReflection[0] = 0.1;
		objList[2]->ambiantReflection[1] = 0.6;
		objList[2]->ambiantReflection[2] = 0.1;
		objList[2]->diffusetReflection[0] = 0.1;
		objList[2]->diffusetReflection[1] = 1;
		objList[2]->diffusetReflection[2] = 0.1;
		objList[2]->specularReflection[0] = 0.3;
		objList[2]->specularReflection[1] = 0.7;
		objList[2]->specularReflection[2] = 0.3;
		objList[2]->speN = 650;

		objList[3]->ambiantReflection[0] = 0.3;
		objList[3]->ambiantReflection[1] = 0.3;
		objList[3]->ambiantReflection[2] = 0.3;
		objList[3]->diffusetReflection[0] = 0.7;
		objList[3]->diffusetReflection[1] = 0.7;
		objList[3]->diffusetReflection[2] = 0.7;
		objList[3]->specularReflection[0] = 0.6;
		objList[3]->specularReflection[1] = 0.6;
		objList[3]->specularReflection[2] = 0.6;
		objList[3]->speN = 650;

	}

	if (i == 1)
	{

		// Step 5
		((Sphere*)objList[0])->set(Vector3(180, 40, -150), 20); //b
		((Sphere*)objList[1])->set(Vector3(150, 40, 80), 80); //y
		((Sphere*)objList[2])->set(Vector3(-130, 40, 0), 90); //g
		((Sphere*)objList[3])->set(Vector3(0, -200, -200), 180); //p

		objList[0]->ambiantReflection[0] = 0.1;
		objList[0]->ambiantReflection[1] = 0.4;
		objList[0]->ambiantReflection[2] = 0.4;
		objList[0]->diffusetReflection[0] = 0;
		objList[0]->diffusetReflection[1] = 0.5;
		objList[0]->diffusetReflection[2] = 0.5;
		objList[0]->specularReflection[0] = 0.0;
		objList[0]->specularReflection[1] = 0.1;
		objList[0]->specularReflection[2] = 0.1;
		objList[0]->speN = 10;

		objList[1]->ambiantReflection[0] = 0.6;
		objList[1]->ambiantReflection[1] = 0.6;
		objList[1]->ambiantReflection[2] = 0.2;
		objList[1]->diffusetReflection[0] = 1;
		objList[1]->diffusetReflection[1] = 1;
		objList[1]->diffusetReflection[2] = 0;
		objList[1]->specularReflection[0] = 0.0;
		objList[1]->specularReflection[1] = 0.0;
		objList[1]->specularReflection[2] = 0.0;
		objList[1]->speN = 650;

		objList[2]->ambiantReflection[0] = 0.1;
		objList[2]->ambiantReflection[1] = 0.6;
		objList[2]->ambiantReflection[2] = 0.1;
		objList[2]->diffusetReflection[0] = 0.1;
		objList[2]->diffusetReflection[1] = 1;
		objList[2]->diffusetReflection[2] = 0.1;
		objList[2]->specularReflection[0] = 0.3;
		objList[2]->specularReflection[1] = 0.7;
		objList[2]->specularReflection[2] = 0.3;
		objList[2]->speN = 300;

		objList[3]->ambiantReflection[0] = 0.3;
		objList[3]->ambiantReflection[1] = 0.3;
		objList[3]->ambiantReflection[2] = 0.3;
		objList[3]->diffusetReflection[0] = 0.7;
		objList[3]->diffusetReflection[1] = 0.7;
		objList[3]->diffusetReflection[2] = 0.7;
		objList[3]->specularReflection[0] = 0.6;
		objList[3]->specularReflection[1] = 0.6;
		objList[3]->specularReflection[2] = 0.6;
		objList[3]->speN = 650;

	}
}

void keyboard (unsigned char key, int x, int y)
{
	//keys to control scaling - k
	//keys to control rotation - alpha
	//keys to control translation - tx, ty
	switch (key) {
	case 's':
	case 'S':
		sceneNo = (sceneNo + 1 ) % NUM_SCENE;
		setScene(sceneNo);
		renderScene();
		glutPostRedisplay();
		break;
	case 'q':
	case 'Q':
		exit(0);

		default:
		break;
	}
}

int main(int argc, char **argv)
{

	
	cout<<"<<CS3241 Lab 5>>\n\n"<<endl;
	cout << "S to go to next scene" << endl;
	cout << "Q to quit"<<endl;
	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize (WINWIDTH, WINHEIGHT);

	glutCreateWindow ("CS3241 Lab 5: Ray Tracing");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	glutKeyboardFunc(keyboard);

	objList = new RtObject*[NUM_OBJECTS];

	// create four spheres
	objList[0] = new Sphere(Vector3(-130, 80, 120), 100);
	objList[1] = new Sphere(Vector3(130, -80, -80), 100);
	objList[2] = new Sphere(Vector3(-130, -80, -80), 100);
	objList[3] = new Sphere(Vector3(130, 80, 120), 100);

	setScene(0);

	setScene(sceneNo);
	renderScene();

	glutMainLoop();

	for (int i = 0; i < NUM_OBJECTS; i++)
		delete objList[i];
	delete[] objList;

	delete[] pixelBuffer;

	return 0;
}
